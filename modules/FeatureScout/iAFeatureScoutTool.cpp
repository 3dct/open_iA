// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFeatureScoutTool.h"

#include "dlg_FeatureScout.h"
#include "iAFeatureScoutToolbar.h"

#include <iACsvConfig.h>
#include "iALabeledVolumeVis.h"
#include <iAObjectsData.h>
#include <iAObjectsViewer.h>
#include <iAObjectVisFactory.h>

#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h>
#include <iAVolumeViewer.h>
#include <iAVolumeRenderer.h>

#include <iAFileUtils.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAToolsVTK.h>    // for RenderModeMap
#include <iATransferFunction.h>

#include <vtkImageData.h>
#include <vtkSmartVolumeMapper.h>

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
	auto objData = loadObjectsCSV(csvConfig);
	if (!objData)
	{
		return false;
	}
	init(csvConfig.objectType, csvConfig.fileName, objData);

	iAFeatureScoutToolbar::addForChild(m_mainWindow, child);
	LOG(lvlInfo, QString("FeatureScout started (csv: %1)").arg(csvConfig.fileName));
	if (csvConfig.visType == iAObjectVisType::UseVolume)
	{
		QVariantMap renderSettings;
		renderSettings[iADataSetRenderer::Shading] = true;
		renderSettings[iADataSetRenderer::DiffuseLighting] = 1.6;
		renderSettings[iADataSetRenderer::SpecularLighting] = 0.0;
		renderSettings[iAVolumeRenderer::Interpolation] = iAVolumeRenderer::InterpolateLinear;
		for (auto key: RenderModeMap().keys())
		{
			if (RenderModeMap()[key] == vtkSmartVolumeMapper::RayCastRenderMode)
			{
				renderSettings[iAVolumeRenderer::RendererType] = key;
				break;
			}
		}
		
		child->dataSetViewer(child->firstImageDataSetIdx())->setAttributes(renderSettings);
	}
	setOptions(csvConfig);
	return true;
}

void iAFeatureScoutTool::init(int objectType, QString const& fileName, std::shared_ptr<iAObjectsData> objData)
{
	double* bounds = nullptr;
	iAObjectVis* objVis = nullptr;
	if (objData->m_visType == iAObjectVisType::UseVolume)
	{
		auto idx = m_child->firstImageDataSetIdx();
		if (idx == iAMdiChild::NoDataSet)
		{
			LOG(lvlError, "No image data set loaded!");
			return;
		}
		auto tf = dynamic_cast<iAVolumeViewer*>(m_child->dataSetViewer(idx))->transfer();
		bounds = dynamic_cast<iAImageData*>(m_child->dataSet(idx).get())->vtkImage()->GetBounds();
		m_objData = objData;
		QColor defaultColor(dlg_FeatureScout::UnclassifiedColorName);
		m_objVis = std::make_shared<iALabeledVolumeVis>(tf->colorTF(), tf->opacityTF(), objData.get(), bounds);
		objVis = m_objVis.get();
	}
	else
	{
		auto objDataSetIdx = m_child->addDataSet(objData);
		connect(m_child, &iAMdiChild::dataSetRemoved, [this, objDataSetIdx](size_t removedDataSetIdx) {
			if (objDataSetIdx == removedDataSetIdx)
			{   // FeatureScout will not work if object dataset is removed, it depends on object visualization being available
				m_child->removeTool(iAFeatureScoutTool::ID);
			}
			});
		auto viewer = dynamic_cast<iAObjectsViewer*>(m_child->dataSetViewer(objDataSetIdx));
		objVis = viewer->objectVis();
	}
	m_featureScout = new dlg_FeatureScout(m_child, static_cast<iAObjectType>(objectType), fileName, objData.get(), objVis);
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
