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
#include "iAGEMSeTool.h"

#include "dlg_GEMSeControl.h"
#include "dlg_GEMSe.h"
#include "dlg_samplings.h"
#include "iAGEMSeModuleInterface.h"
#include "iAGEMSeTool.h"

#include <dlg_modalities.h>
#include <iAColorTheme.h>
#include <iALog.h>
//#include <iALogger.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
//#include <iAModality.h>
#include <iAModuleDispatcher.h> // TODO: Refactor; it shouldn't be required to go via iAModuleDispatcher to retrieve one's own module
//#include <iARenderer.h>
//#include <iASlicer.h>

const QString iAGEMSeTool::ID("GEMSe");

iAGEMSeTool::iAGEMSeTool(iAMainWindow* mainWnd, iAMdiChild* child):
	iATool(mainWnd, child),
	m_dummyTitleWidget(new QWidget())
{
}

void iAGEMSeTool::loadState(QSettings & projectFile, QString const & fileName)
{
	iAGEMSeModuleInterface * gemseModule = m_mainWindow->moduleDispatcher().module<iAGEMSeModuleInterface>();
	gemseModule->loadProject(m_child, projectFile, fileName);
}

void iAGEMSeTool::saveState(QSettings & projectFile, QString const & fileName)
{
	iAGEMSeModuleInterface * gemseModule = m_mainWindow->moduleDispatcher().module<iAGEMSeModuleInterface>();
	gemseModule->saveProject(projectFile, fileName);
}

std::shared_ptr<iATool> iAGEMSeTool::create(iAMainWindow* mainWnd, iAMdiChild* child)
{
	auto t = std::make_shared<iAGEMSeTool>(mainWnd, child);
	QString defaultThemeName("Brewer Set3 (max. 12)");
	iAColorTheme const* colorTheme = iAColorThemeManager::instance().theme(defaultThemeName);

	t->m_dlgGEMSe = new dlg_GEMSe(child, iALog::get(), colorTheme);
	t->m_dlgSamplings = new dlg_samplings();
	t->m_dlgGEMSeControl = new dlg_GEMSeControl(
		child,
		t->m_dlgGEMSe,
		child->dataDockWidget(),
		t->m_dlgSamplings,
		colorTheme
	);
	child->splitDockWidget(child->renderDockWidget(), t->m_dlgGEMSe, Qt::Vertical);
	child->splitDockWidget(child->renderDockWidget(), t->m_dlgGEMSeControl, Qt::Horizontal);
	child->splitDockWidget(t->m_dlgGEMSeControl, t->m_dlgSamplings, Qt::Vertical);
	return t;
}

bool iAGEMSeTool::loadSampling(QString const& smpFileName, int labelCount, int datasetID)
{
	return m_dlgGEMSeControl->loadSampling(smpFileName, labelCount, datasetID);
}

bool iAGEMSeTool::loadClustering(QString const& fileName)
{
	return m_dlgGEMSeControl->loadClustering(fileName);
}

bool iAGEMSeTool::loadRefImg(QString const& refImgName)
{
	return m_dlgGEMSeControl->loadRefImg(refImgName);
}

void iAGEMSeTool::setSerializedHiddenCharts(QString const& hiddenCharts)
{
	return m_dlgGEMSeControl->setSerializedHiddenCharts(hiddenCharts);
}

void iAGEMSeTool::resetFilter()
{
	m_dlgGEMSe->ResetFilters();
}

void iAGEMSeTool::toggleAutoShrink()
{
	m_dlgGEMSe->ToggleAutoShrink();
}

void iAGEMSeTool::toggleDockWidgetTitleBar()
{
	QWidget* titleBar = m_dlgGEMSe->titleBarWidget();
	if (titleBar == m_dummyTitleWidget)
	{
		m_dlgGEMSe->setTitleBarWidget(nullptr);
	}
	else
	{
		m_dlgGEMSe->setTitleBarWidget(m_dummyTitleWidget);
	}
}

void iAGEMSeTool::exportClusterIDs()
{
	m_dlgGEMSeControl->exportIDs();
}

void iAGEMSeTool::exportAttributeRangeRanking()
{
	m_dlgGEMSeControl->exportAttributeRangeRanking();
}

void iAGEMSeTool::exportRankings()
{
	m_dlgGEMSeControl->exportRankings();
}

void iAGEMSeTool::importRankings()
{
	m_dlgGEMSeControl->importRankings();
}

void iAGEMSeTool::setLabelInfo(QString const& colorTheme, QString const& labelNames)
{
	m_dlgGEMSeControl->setLabelInfo(colorTheme, labelNames);
}

void iAGEMSeTool::saveProject(QSettings& metaFile, QString const& fileName)
{
	m_dlgGEMSeControl->saveProject(metaFile, fileName);
}
