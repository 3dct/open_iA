// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFeatureScoutTool.h"

#include "dlg_FeatureScout.h"
#include "iAFeatureScoutToolbar.h"

#include <iAObjectVisFactory.h>
#include <iACsvConfig.h>
#include <iACsvIO.h>
#include <iACsvVtkTableCreator.h>

#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h>
#include <iAVolumeViewer.h>

#include <iADataSet.h>
#include <iAFileUtils.h>
#include <iALog.h>
#include <iAToolsVTK.h>    // for RenderModeMap
#include <iATransferFunction.h>

#include <vtkImageData.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkTable.h>

#include <QFileInfo>
#include <QSettings>
#include <QTextStream>

std::shared_ptr<iATool> iAFeatureScoutTool::create(iAMainWindow* mainWnd, iAMdiChild* child)
{
	return std::make_shared<iAFeatureScoutTool>(mainWnd, child);
}

iAFeatureScoutTool::iAFeatureScoutTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_featureScout(nullptr)
{
}

iAFeatureScoutTool::~iAFeatureScoutTool()
{
	delete m_featureScout;
}

bool iAFeatureScoutTool::addToChild(iAMdiChild* child, const QString& csvFileName)
{
	if (csvFileName.isEmpty())
	{
		return false;
	}
	auto type = guessFeatureType(csvFileName);
	if (type == InvalidObjectType)
	{
		LOG(lvlError, "CSV-file could not be opened or not a valid FeatureScout file!");
		return false;
	}
	iACsvConfig csvConfig = (type != Voids) ? iACsvConfig::getFCPFiberFormat(csvFileName)
											: iACsvConfig::getFCVoidFormat(csvFileName);
	return addToChild(child, csvConfig);
}

bool iAFeatureScoutTool::addToChild(iAMdiChild* child, iACsvConfig const& csvConfig)
{
	auto tool = std::make_shared<iAFeatureScoutTool>(iAMainWindow::get(), child);
	auto result = tool->initFromConfig(child, csvConfig);
	if (result)
	{
		child->addTool(iAFeatureScoutTool::ID, tool);
	}
	return result;
}

const QString iAFeatureScoutTool::ID("FeatureScout");

void iAFeatureScoutTool::loadState(QSettings & projectFile, QString const& fileName)
{
	if (!m_child)
	{
		LOG(lvlError,
			QString("Invalid FeatureScout project file '%1': FeatureScout requires a child window, "
					"but UseMdiChild was apparently not specified in this project, as no child window available! "
					"Please report this error, along with the project file, to the open_iA developers!")
				.arg(fileName));
		return;
	}
	m_config.load(projectFile, "CSVFormat");

	QString path(QFileInfo(fileName).absolutePath());
	QString csvFileName = projectFile.value("CSVFileName").toString();
	if (csvFileName.isEmpty())
	{
		LOG(lvlError, QString("Invalid FeatureScout project file '%1': Empty or missing 'CSVFileName'!").arg(fileName));
		return;
	}
	m_config.fileName = MakeAbsolute(path, csvFileName);
	if (projectFile.contains("CurvedFileName") && !projectFile.value("CurvedFileName").toString().isEmpty())
	{
		m_config.curvedFiberFileName = MakeAbsolute(path, projectFile.value("CurvedFileName").toString());
	}
	if (!initFromConfig(m_child, m_config))
	{
		LOG(lvlError, "Error while initializing FeatureScout!");
		return;
	}
	QString layoutName = projectFile.value("Layout").toString();
	if (!layoutName.isEmpty())
	{
		m_child->loadLayout(layoutName);
	}
	m_featureScout->loadProject(projectFile);
}

void iAFeatureScoutTool::saveState(QSettings& projectFile, QString const& fileName)
{
	m_config.save(projectFile, "CSVFormat");
	QString path(QFileInfo(fileName).absolutePath());
	projectFile.setValue("CSVFileName", MakeRelative(path, m_config.fileName));
	if (!m_config.curvedFiberFileName.isEmpty())
	{
		projectFile.setValue("CurvedFileName", MakeRelative(path, m_config.curvedFiberFileName));
	}
	if (m_child)
	{
		projectFile.setValue("Layout", m_child->layoutName());
	}
	m_featureScout->saveProject(projectFile);
}

bool iAFeatureScoutTool::initFromConfig(iAMdiChild* child, iACsvConfig const& csvConfig)
{
	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
	{
		return false;
	}
	std::map<size_t, std::vector<iAVec3f>> curvedFiberInfo;
	if (!csvConfig.curvedFiberFileName.isEmpty())
	{
		readCurvedFiberInfo(csvConfig.curvedFiberFileName, curvedFiberInfo);
	}
	init(csvConfig.objectType, csvConfig.fileName, creator.table(), csvConfig.visType, io.getOutputMapping(),
		curvedFiberInfo, csvConfig.cylinderQuality, csvConfig.segmentSkip);

	iAFeatureScoutToolbar::addForChild(m_mainWindow, child);
	LOG(lvlInfo, QString("FeatureScout started (csv: %1)").arg(csvConfig.fileName));
	if (csvConfig.visType == iAObjectVisType::UseVolume)
	{
		QVariantMap renderSettings;
		// ToDo: Remove duplication with string constants from iADataSetRenderer!
		renderSettings["Shading"] = true;
		renderSettings["Linear interpolation"] = false;
		renderSettings["Diffuse lighting"] = 1.6;
		renderSettings["Specular lighting"] = 0.0;
		for (auto key: RenderModeMap().keys())
		{
			if (RenderModeMap()[key] == vtkSmartVolumeMapper::RayCastRenderMode)
			{
				renderSettings["Renderer type"] = key;
				break;
			}
		}
		
		child->dataSetViewer(child->firstImageDataSetIdx())->setAttributes(renderSettings);
	}
	setOptions(csvConfig);
	return true;
}

void iAFeatureScoutTool::init(int filterID, QString const& fileName, vtkSmartPointer<vtkTable> csvtbl,
	iAObjectVisType visType, QSharedPointer<QMap<uint, uint> > columnMapping, std::map<size_t,
	std::vector<iAVec3f> > & curvedFiberInfo, int cylinderQuality, size_t segmentSkip)
{
	vtkColorTransferFunction* ctf = nullptr;
	vtkPiecewiseFunction* otf = nullptr;
	double* bounds = nullptr;
	if (visType == iAObjectVisType::UseVolume)
	{
		auto idx = m_child->firstImageDataSetIdx();
		if (idx == iAMdiChild::NoDataSet)
		{
			LOG(lvlError, "No image data set loaded!");
			return;
		}
		auto tf = dynamic_cast<iAVolumeViewer*>(m_child->dataSetViewer(idx))->transfer();
		ctf = tf->colorTF();
		otf = tf->opacityTF();
		bounds = dynamic_cast<iAImageData*>(m_child->dataSet(idx).get())->vtkImage()->GetBounds();
	}
	auto objvis = create3DObjectVis(visType, csvtbl, columnMapping,
		QColor(dlg_FeatureScout::UnclassifiedColorName), curvedFiberInfo, cylinderQuality, segmentSkip,
		ctf, otf, bounds);
	m_featureScout = new dlg_FeatureScout(
		m_child, static_cast<iAObjectType>(filterID),
		fileName, csvtbl, visType, columnMapping, objvis);
}

void iAFeatureScoutTool::setOptions(iACsvConfig const& config)
{
	m_config = config;
}

iAObjectType iAFeatureScoutTool::guessFeatureType(QString const& csvFileName)
{
	QFile file(csvFileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return InvalidObjectType;
	}
	// Automatic csv file detection - 2nd line of pore csv file have this line; fibers csvs not yet
	QTextStream in(&file);
	in.readLine();
	QString item = in.readLine();
	file.close();
	return (item == "Voids") ? Voids : Fibers;
}
