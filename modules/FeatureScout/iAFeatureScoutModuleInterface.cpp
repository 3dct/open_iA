/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAFeatureScoutModuleInterface.h"

#include "iAFeatureScoutTool.h"
#include "iAFeatureScoutToolbar.h"

#include <dlg_CSVInput.h>
#include <iACsvConfig.h>
#include <iACsvIO.h>
#include <iACsvVtkTableCreator.h>

#include <iAFileUtils.h>
#include <iALog.h>
#include <iAModalityList.h>
#include <iAModuleDispatcher.h> // TODO: Refactor; it shouldn't be required to go via iAModuleDispatcher to retrieve one's own module
#include <iAToolRegistry.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iARenderSettings.h>
#include <iAVolumeSettings.h>
#include <iAToolsVTK.h>

#include <vtkTable.h>
#include <vtkSmartVolumeMapper.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QStatusBar>
#include <QTextStream>


namespace
{
	iAObjectType guessFeatureType(QString const& csvFileName)
	{
		QFile file(csvFileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return InvalidObjectType;
		}
		// TODO: create convention, 2nd line of csv file for fibers (pore csv file have this line)
		// Automatic csv file detection
		QTextStream in(&file);
		in.readLine();
		QString item = in.readLine();
		auto returnType = (item == "Voids") ? Voids : Fibers;
		file.close();
		return returnType;
	}
}

void iAFeatureScoutModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	Q_INIT_RESOURCE(FeatureScout);

	iAToolRegistry::addTool(iAFeatureScoutTool::ID, iAFeatureScoutTool::create);
	QAction * actionFibreScout = new QAction(tr("FeatureScout"), m_mainWnd);
	connect(actionFibreScout, &QAction::triggered, this, &iAFeatureScoutModuleInterface::featureScout);
	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Feature Analysis"), true);
	submenu->addAction(actionFibreScout);
}

void iAFeatureScoutModuleInterface::featureScout()
{
	bool volumeDataAvailable = m_mainWnd->activeMdiChild() &&
		m_mainWnd->activeMdiChild()->firstImageData() != nullptr;
	dlg_CSVInput dlg(volumeDataAvailable);
	if (m_mainWnd->activeMdiChild())
	{
		auto mdi = m_mainWnd->activeMdiChild();
		QString testCSVFileName = pathFileBaseName(mdi->fileInfo()) + ".csv";
		if (QFile(testCSVFileName).exists())
		{
			dlg.setFileName(testCSVFileName);
			auto type = guessFeatureType(testCSVFileName);
			if (type != InvalidObjectType)
			{
				dlg.setFormat(type == Voids ? iACsvConfig::FCVoidFormat : iACsvConfig::FCPFiberFormat);
			}
		}
		else
		{
			dlg.setPath(mdi->filePath());
		}
	}
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	iACsvConfig csvConfig = dlg.getConfig();
	bool createdMdi = false;
	iAMdiChild* child = nullptr;
	if (csvConfig.visType != iACsvConfig::UseVolume)
	{
		if (m_mainWnd->activeMdiChild() && QMessageBox::question(m_mainWnd, "FeatureScout",
			"Load FeatureScout in currently active window (If you choose No, FeatureScout will be opened in a new window)?",
			QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
		{
			child = m_mainWnd->activeMdiChild();
		}
		else
		{
			createdMdi = true;
			child = m_mainWnd->createMdiChild(false);
		}
	}
	else
	{
		child = m_mainWnd->activeMdiChild();
	}
	if (!startFeatureScout(child, csvConfig) && createdMdi)
	{
		m_mainWnd->closeMdiChild(child);
		QMessageBox::warning(m_mainWnd, "FeatureScout", "Starting FeatureScout failed! Please check console for detailed error messages!");
	}
}

void iAFeatureScoutModuleInterface::startFeatureScoutWithParams(iAMdiChild* child, QString const& csvFileName)
{
	if (csvFileName.isEmpty())
	{
		return;
	}
	auto type = guessFeatureType(csvFileName);
	if (type == InvalidObjectType)
	{
		LOG(lvlError, "CSV-file could not be opened or not a valid FeatureScout file!");
		return;
	}
	iACsvConfig csvConfig = (type != Voids) ?
		iACsvConfig::getFCPFiberFormat( csvFileName ):
		iACsvConfig::getFCVoidFormat( csvFileName );
	startFeatureScout(child, csvConfig);
}

bool iAFeatureScoutModuleInterface::startFeatureScout(iAMdiChild* child, iACsvConfig const& csvConfig)
{
	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
	{
		return false;
	}
	auto tool = std::make_shared<iAFeatureScoutTool>(m_mainWnd, child);
	child->addTool(iAFeatureScoutTool::ID, tool);
	std::map<size_t, std::vector<iAVec3f> > curvedFiberInfo;
	if (!csvConfig.curvedFiberFileName.isEmpty())
	{
		readCurvedFiberInfo(csvConfig.curvedFiberFileName, curvedFiberInfo);
	}
	tool->init(csvConfig.objectType, csvConfig.fileName, creator.table(), csvConfig.visType, io.getOutputMapping(),
		curvedFiberInfo, csvConfig.cylinderQuality, csvConfig.segmentSkip);

	iAFeatureScoutToolbar::addForChild(m_mainWnd, child);
	child->addStatusMsg(QString("FeatureScout started (csv: %1)").arg(csvConfig.fileName));
	LOG(lvlInfo, QString("FeatureScout started (csv: %1)").arg(csvConfig.fileName));
	if (csvConfig.visType == iACsvConfig::UseVolume)
	{
		QVariantMap renderSettings;
		// ToDo: Remove duplication with string constants from iADataSetRenderer!
		renderSettings["Shading"] = true;
		renderSettings["Linear interpolation"] = false;
		renderSettings["Diffuse lighting"] = 1.6;
		renderSettings["Specular lighting"] = 0.0;
		renderSettings["Renderer type"] = RenderModeMap()[vtkSmartVolumeMapper::RayCastRenderMode];

		child->applyRenderSettings(child->firstImageDataSetIdx(), renderSettings);
	}
	tool->setOptions(csvConfig);
	child->addTool(iAFeatureScoutTool::ID, tool);
	return true;
}
