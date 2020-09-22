/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAParameterExplorerModuleInterface.h"

#include "iAParameterExplorerAttachment.h"

#include <iAConsole.h>
#include <io/iAFileUtils.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <QFileDialog>
#include <QSettings>

void iAParameterExplorerModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction * actionExplore = new QAction(tr("Parameter Explorer"), m_mainWnd);
	makeActionChildDependent(actionExplore);
	connect(actionExplore, &QAction::triggered, this, &iAParameterExplorerModuleInterface::StartParameterExplorer);

	QAction * actionLoad = new QAction(tr("Load Parameter Explorer State"), m_mainWnd);
	makeActionChildDependent(actionLoad);
	connect(actionLoad, &QAction::triggered, this, &iAParameterExplorerModuleInterface::LoadState);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Image Ensembles"), true);
	submenu->addAction(actionExplore);
	submenu->addAction(actionLoad);
}

void iAParameterExplorerModuleInterface::SetupToolBar()
{
	if (m_toolBar)
	{
		return;
	}
	m_toolBar = new iAParamToolBar("Parameter Explorer Toolbar");
	m_toolBar->action_ToggleTitleBar->setCheckable(true);
	m_toolBar->action_ToggleTitleBar->setChecked(true);
	connect(m_toolBar->action_ToggleTitleBar, &QAction::triggered, this, &iAParameterExplorerModuleInterface::ToggleDockWidgetTitleBars);
	m_toolBar->action_ToggleSettings->setCheckable(true);
	m_toolBar->action_ToggleSettings->setChecked(true);
	connect(m_toolBar->action_ToggleSettings, &QAction::triggered, this, &iAParameterExplorerModuleInterface::ToggleSettings);
	connect(m_toolBar->action_SaveState, &QAction::triggered, this, &iAParameterExplorerModuleInterface::SaveState);
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolBar);
}

void iAParameterExplorerModuleInterface::ToggleDockWidgetTitleBars()
{
	m_mdiChild = m_mainWnd->activeMdiChild();
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
	m_mdiChild = m_mainWnd->activeMdiChild();
	iAParameterExplorerAttachment* attach = GetAttachment<iAParameterExplorerAttachment>();
	if (!attach)
	{
		DEBUG_LOG("ParameterExplorer was not loaded properly!");
		return;
	}
	attach->ToggleSettings(m_toolBar->action_ToggleSettings->isChecked());
}

void iAParameterExplorerModuleInterface::StartParameterExplorer()
{
	PrepareActiveChild();
	if (!m_mdiChild)
	{
		return;
	}
	QString csvFileName = QFileDialog::getOpenFileName(m_mainWnd,
		tr("Select CSV File"), m_mdiChild->filePath(), tr("CSV Files (*.csv);;"));
	if (csvFileName.isEmpty())
	{
		return;
	}
	CreateAttachment(csvFileName, m_mdiChild);
}

void iAParameterExplorerModuleInterface::SaveState()
{
	m_mdiChild = m_mainWnd->activeMdiChild();
	iAParameterExplorerAttachment* attach = GetAttachment<iAParameterExplorerAttachment>();
	if (!attach)
	{
		DEBUG_LOG("ParameterExplorer was not loaded properly!");
		return;
	}
	QString stateFileName = QFileDialog::getSaveFileName(m_mainWnd, "Save Parameter Explorer State",
		m_mdiChild->filePath(), "Parameter Explorer State (*.pes);;");
	if (stateFileName.isEmpty())
	{
		return;
	}
	QSettings stateFileSettings(stateFileName, QSettings::IniFormat);
	QFileInfo stateFileInfo(stateFileName);
	stateFileSettings.setValue("Reference", MakeRelative(stateFileInfo.absolutePath(), m_mdiChild->currentFile()));
	stateFileSettings.setValue("CSVFile", MakeRelative(stateFileInfo.absolutePath(), attach->CSVFileName()));
	stateFileSettings.setValue("Layout", m_mdiChild->layoutName());
	attach->SaveSettings(stateFileSettings);
}

void iAParameterExplorerModuleInterface::LoadState()
{
	QString stateFileName = QFileDialog::getOpenFileName(m_mainWnd, "Save Parameter Explorer State",
		"", "Parameter Explorer State (*.pes);;");
	if (stateFileName.isEmpty())
	{
		return;
	}
	QFileInfo stateFileInfo(stateFileName);
	QSettings stateFileSettings(stateFileName, QSettings::IniFormat);
	QString refFileName = MakeAbsolute(stateFileInfo.absolutePath(), stateFileSettings.value("Reference").toString());
	MdiChild *child = m_mainWnd->createMdiChild(false);
	m_stateFiles.insert(child, stateFileName);
	connect(child, &MdiChild::fileLoaded, this, &iAParameterExplorerModuleInterface::ContinueStateLoading);
	if (!child->loadFile(refFileName, false))
	{
		DEBUG_LOG(QString("Could not load reference file %1.").arg(refFileName));
		return;
	}
}

void iAParameterExplorerModuleInterface::ContinueStateLoading()
{
	MdiChild* child = dynamic_cast<MdiChild*>(QObject::sender());
	m_mdiChild = child;
	iAParameterExplorerAttachment* attach = GetAttachment<iAParameterExplorerAttachment>();
	if (!child || attach)
	{
		DEBUG_LOG("ParameterExplorer: Invalid state - child null or Parameter Explorer already attached!");
		return;
	}
	QString stateFileName = m_stateFiles[child];
	QFileInfo stateFileInfo(stateFileName);
	QSettings stateFileSettings(stateFileName, QSettings::IniFormat);
	QString csvFileName = MakeAbsolute(stateFileInfo.absolutePath(), stateFileSettings.value("CSVFile").toString());
	if (!CreateAttachment(csvFileName, child))
	{
		return;
	}
	attach = GetAttachment<iAParameterExplorerAttachment>();
	if (!attach)
	{
		DEBUG_LOG("ParameterExplorer was not loaded properly!");
		return;
	}
	attach->LoadSettings(stateFileSettings);
	child->showMaximized();
	child->loadLayout(stateFileSettings.value("Layout").toString());
	m_stateFiles.remove(child);
}

bool iAParameterExplorerModuleInterface::CreateAttachment(QString const & csvFileName, MdiChild* child)
{
	bool result = AttachToMdiChild(child);
	m_mdiChild = child;
	if (!result)
	{
		return false;
	}
	iAParameterExplorerAttachment* attach = GetAttachment<iAParameterExplorerAttachment>();
	if (!attach)
	{
		DEBUG_LOG("ParameterExplorer was not loaded properly!");
		return false;
	}
	attach->LoadCSV(csvFileName);
	SetupToolBar();
	return true;
}

iAModuleAttachmentToChild* iAParameterExplorerModuleInterface::CreateAttachment(MainWindow* mainWnd, MdiChild * child)
{
	return iAParameterExplorerAttachment::create( mainWnd, child);
}
