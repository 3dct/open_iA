/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "pch.h"
#include "iAParameterExplorerModuleInterface.h"

#include "iAParameterExplorerAttachment.h"

#include "iAConsole.h"
#include "iAChildData.h"
#include "mainwindow.h"

void iAParameterExplorerModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;

	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuEnsembles = getMenuWithTitle( toolsMenu, QString( "Image Ensembles" ), false );
	QAction * actionExplore = new QAction( m_mainWnd );
	actionExplore->setText(QApplication::translate("MainWindow", "Parameter Explorer", 0));
	AddActionToMenuAlphabeticallySorted(menuEnsembles, actionExplore, true);
	connect(actionExplore, SIGNAL(triggered()), this, SLOT(StartParameterExplorer()));
}



void iAParameterExplorerModuleInterface::SetupToolBar()
{
	if (m_toolBar)
		return;
	m_toolBar = new iAParamToolBar("Parameter Explorer Toolbar");
	m_toolBar->action_ToggleTitleBar->setCheckable(true);
	m_toolBar->action_ToggleTitleBar->setChecked(true);
	connect(m_toolBar->action_ToggleTitleBar, SIGNAL(triggered()), this, SLOT(ToggleDockWidgetTitleBars()));
	m_toolBar->action_ToggleSettings->setCheckable(true);
	m_toolBar->action_ToggleSettings->setChecked(true);
	connect(m_toolBar->action_ToggleSettings, SIGNAL(triggered()), this, SLOT(ToggleSettings()));
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolBar);
}

void iAParameterExplorerModuleInterface::ToggleDockWidgetTitleBars()
{
	iAParameterExplorerAttachment* attach = GetAttachment<iAParameterExplorerAttachment>();
	if (!attach)
	{
		DEBUG_LOG("ParameterExplorer was not loaded properly!");
		return;
	}
	attach->ToggleDockWidgetTitleBars();
}

void iAParameterExplorerModuleInterface::ToggleSettings()
{
	iAParameterExplorerAttachment* attach = GetAttachment<iAParameterExplorerAttachment>();
	if (!attach)
	{
		DEBUG_LOG("ParameterExplorer was not loaded properly!");
		return;
	}
	attach->ToggleSettings();
}

bool iAParameterExplorerModuleInterface::StartParameterExplorer()
{
	PrepareActiveChild();
	if (!m_mdiChild)
	{
		return false;
	}
	SetupToolBar();
	return AttachToMdiChild(m_mdiChild);
}

iAModuleAttachmentToChild* iAParameterExplorerModuleInterface::CreateAttachment(MainWindow* mainWnd, iAChildData childData)
{
	iAParameterExplorerAttachment* result = iAParameterExplorerAttachment::create( mainWnd, childData);
	return result;
}
