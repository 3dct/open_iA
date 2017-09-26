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
#include "iAUncertaintyModuleInterface.h"

#include "iAConsole.h"
#include "iAUncertaintyAttachment.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QFileDialog>


void iAUncertaintyModuleInterface::Initialize()
{
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuSegmentation = getMenuWithTitle( toolsMenu, QString( "Segmentation Ensembles" ), false );
	
	QAction * actionUncertainty = new QAction(QApplication::translate("MainWindow", "Uncertainty Exploration", 0), m_mainWnd );
	AddActionToMenuAlphabeticallySorted(menuSegmentation, actionUncertainty, false);
	connect(actionUncertainty, SIGNAL(triggered()), this, SLOT(UncertaintyExploration()));
}


iAModuleAttachmentToChild* iAUncertaintyModuleInterface::CreateAttachment(MainWindow* mainWnd, iAChildData childData)
{
	iAUncertaintyAttachment* result = iAUncertaintyAttachment::Create( mainWnd, childData);
	return result;
}


void iAUncertaintyModuleInterface::UncertaintyExploration()
{
	QString fileName = QFileDialog::getOpenFileName(m_mainWnd,
		tr("Load Ensemble"),
		m_mainWnd->activeMdiChild() ? m_mainWnd->activeMdiChild()->getFilePath(): QString(),
		tr("Image Analysis Ensemble (*.iae );;") );
	if (fileName != "")
	{
		LoadEnsemble(fileName);
	}
}

void iAUncertaintyModuleInterface::LoadEnsemble(QString const & fileName)
{
	SetupToolBar();
	m_mdiChild = m_mainWnd->createMdiChild(false);
	UpdateChildData();
	bool result = AttachToMdiChild(m_mdiChild);
	iAUncertaintyAttachment* attach = GetAttachment<iAUncertaintyAttachment>();
	if (!result || !attach)
	{
		DEBUG_LOG("Uncertainty exploration could not be initialized!");
		return;
	}
	m_mdiChild->show();
	if (!attach->LoadEnsemble(fileName))
	{
		delete m_mdiChild;
		return;
	}
}

void iAUncertaintyModuleInterface::SetupToolBar()
{
	if (m_toolbar)
	{
		return;
	}
	m_toolbar = new iAUncertaintyToolbar("Uncertainty Exploration Toolbar");
	connect(m_toolbar->action_ToggleTitleBar, SIGNAL(triggered()), this, SLOT(ToggleDockWidgetTitleBars()));
	m_toolbar->action_ToggleSettings->setCheckable(true);
	m_toolbar->action_ToggleSettings->setChecked(true);
	connect(m_toolbar->action_ToggleSettings, SIGNAL(triggered()), this, SLOT(ToggleSettings()));
	connect(m_toolbar->action_CalculateNewSubEnsemble, SIGNAL(triggered()), this, SLOT(CalculateNewSubEnsemble()));
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolbar);
}

void iAUncertaintyModuleInterface::ToggleDockWidgetTitleBars()
{
	iAUncertaintyAttachment* attach = GetAttachment<iAUncertaintyAttachment>();
	if (!attach)
	{
		DEBUG_LOG("Uncertainty exploration was not loaded properly!");
		return;
	}
	attach->ToggleDockWidgetTitleBars();
}

void iAUncertaintyModuleInterface::ToggleSettings()
{
	iAUncertaintyAttachment* attach = GetAttachment<iAUncertaintyAttachment>();
	if (!attach)
	{
		DEBUG_LOG("Uncertainty exploration was not loaded properly!");
		return;
	}
	attach->ToggleSettings();
}

void iAUncertaintyModuleInterface::CalculateNewSubEnsemble()
{
	iAUncertaintyAttachment* attach = GetAttachment<iAUncertaintyAttachment>();
	if (!attach)
	{
		DEBUG_LOG("Uncertainty exploration was not loaded properly!");
		return;
	}
	attach->CalculateNewSubEnsemble();
}
