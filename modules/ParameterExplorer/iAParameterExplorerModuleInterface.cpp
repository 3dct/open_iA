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
#include "iAParameterExplorerModuleInterface.h"

#include "iAParameterExplorerAttachment.h"

#include <iALog.h>
#include <iAFileUtils.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QSettings>

void iAParameterExplorerModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction * actionExplore = new QAction(tr("Parameter Explorer"), m_mainWnd);
	m_mainWnd->makeActionChildDependent(actionExplore);
	connect(actionExplore, &QAction::triggered, this, &iAParameterExplorerModuleInterface::StartParameterExplorer);

	QAction * actionLoad = new QAction(tr("Load Parameter Explorer State"), m_mainWnd);
	m_mainWnd->makeActionChildDependent(actionLoad);
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
	iAParameterExplorerAttachment* attach = attachment<iAParameterExplorerAttachment>(m_mainWnd->activeMdiChild());
	if (!attach)
	{
		LOG(lvlError, "ParameterExplorer was not loaded properly!");
		return;
	}
	attach->ToggleDockWidgetTitleBars();
}

void iAParameterExplorerModuleInterface::ToggleSettings()
{
	iAParameterExplorerAttachment* attach = attachment<iAParameterExplorerAttachment>(m_mainWnd->activeMdiChild());
	if (!attach)
	{
		LOG(lvlError, "ParameterExplorer was not loaded properly!");
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
		tr("Select CSV File"), m_mdiChild->filePath(), tr("CSV Files (*.csv);;All files (*)"));
	if (csvFileName.isEmpty())
	{
		return;
	}
	CreateAttachment(csvFileName, m_mdiChild);
}

void iAParameterExplorerModuleInterface::SaveState()
{
	m_mdiChild = m_mainWnd->activeMdiChild();
	iAParameterExplorerAttachment* attach = attachment<iAParameterExplorerAttachment>(m_mainWnd->activeMdiChild());
	if (!attach)
	{
		LOG(lvlError, "ParameterExplorer was not loaded properly!");
		return;
	}
	QString stateFileName = QFileDialog::getSaveFileName(m_mainWnd, "Save Parameter Explorer State",
		m_mdiChild->filePath(), "Parameter Explorer State (*.pes);;All files (*)");
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
		"", "Parameter Explorer State (*.pes);;All files (*)");
	if (stateFileName.isEmpty())
	{
		return;
	}
	QFileInfo stateFileInfo(stateFileName);
	QSettings stateFileSettings(stateFileName, QSettings::IniFormat);
	QString refFileName = MakeAbsolute(stateFileInfo.absolutePath(), stateFileSettings.value("Reference").toString());
	iAMdiChild* child = m_mainWnd->createMdiChild(false);
	m_stateFiles.insert(child, stateFileName);
	connect(child, &iAMdiChild::fileLoaded, this, &iAParameterExplorerModuleInterface::ContinueStateLoading);
	if (!child->loadFile(refFileName, false))
	{
		LOG(lvlError, QString("Could not load reference file %1.").arg(refFileName));
		return;
	}
}

void iAParameterExplorerModuleInterface::ContinueStateLoading()
{
	iAMdiChild* child = dynamic_cast<iAMdiChild*>(QObject::sender());
	m_mdiChild = child;
	iAParameterExplorerAttachment* attach = attachment<iAParameterExplorerAttachment>(m_mdiChild);
	if (!child || attach)
	{
		LOG(lvlError, "ParameterExplorer: Invalid state - child null or Parameter Explorer already attached!");
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
	attach = attachment<iAParameterExplorerAttachment>(m_mdiChild);
	if (!attach)
	{
		LOG(lvlError, "ParameterExplorer was not loaded properly!");
		return;
	}
	attach->LoadSettings(stateFileSettings);
	child->showMaximized();
	child->loadLayout(stateFileSettings.value("Layout").toString());
	m_stateFiles.remove(child);
}

bool iAParameterExplorerModuleInterface::CreateAttachment(QString const & csvFileName, iAMdiChild* child)
{
	bool result = AttachToMdiChild(child);
	if (!result)
	{
		return false;
	}
	iAParameterExplorerAttachment* attach = attachment<iAParameterExplorerAttachment>(child);
	if (!attach)
	{
		LOG(lvlError, "ParameterExplorer was not loaded properly!");
		return false;
	}
	attach->LoadCSV(csvFileName);
	SetupToolBar();
	return true;
}

iAModuleAttachmentToChild* iAParameterExplorerModuleInterface::CreateAttachment(iAMainWindow* mainWnd, iAMdiChild * child)
{
	return iAParameterExplorerAttachment::create( mainWnd, child);
}
