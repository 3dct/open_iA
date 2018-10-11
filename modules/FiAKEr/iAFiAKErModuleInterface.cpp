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
#include "iAFiAKErModuleInterface.h"

#include "iACsvConfig.h"
#include "iAFiberCharData.h"
#include "iAFiAKErController.h"

#include "dlg_commoninput.h"
#include "mainwindow.h"

#include <QAction>
#include <QFileDialog>

void iAFiAKErModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QAction * actionFiAKEr = new QAction( "FiAKEr (Fiber Analytics)", nullptr );
	AddActionToMenuAlphabeticallySorted( toolsMenu, actionFiAKEr, false );
	connect(actionFiAKEr, &QAction::triggered, this, &iAFiAKErModuleInterface::FiAKEr );
}

void iAFiAKErModuleInterface::FiAKEr()
{
	QString path = QFileDialog::getExistingDirectory(m_mainWnd, "Choose Folder containing Result csv", m_mainWnd->getPath());
	if (path.isEmpty())
		return;
	
	auto explorer = new iAFiAKErController(m_mainWnd);
	QStringList parameterNames = QStringList() << "+CSV Format";
	QStringList formatEntries = iACsvConfig::getListFromRegistry();
	if (!formatEntries.contains(iAFiberResultsCollection::SimpleFormat))
		formatEntries.append(iAFiberResultsCollection::SimpleFormat);
	if (!formatEntries.contains(iAFiberResultsCollection::LegacyFormat))
		formatEntries.append(iAFiberResultsCollection::LegacyFormat);
	if (!formatEntries.contains(iACsvConfig::LegacyFiberFormat))
		formatEntries.append(iACsvConfig::LegacyFiberFormat);
	if (!formatEntries.contains(iACsvConfig::LegacyVoidFormat))
		formatEntries.append(iACsvConfig::LegacyVoidFormat);
	QList<QVariant> values;
	values << formatEntries;
	dlg_commoninput dlg(m_mainWnd, "Choose CSV Format", parameterNames, values);
	if (dlg.exec() != QDialog::Accepted)
		return;
	QString configName = dlg.getComboBoxValue(0);
	//cmbbox_Format->addItems(formatEntries);
	m_mainWnd->addSubWindow(explorer);
	explorer->start(path, configName);
}

/*
void iAFiAKErModuleInterface::SetupToolBar()
{
	if (m_toolbar)
	{
		return;
	}
	m_toolbar = new iAUncertaintyToolbar("Uncertainty Exploration Toolbar");
	connect(m_toolbar->action_ToggleTitleBar, SIGNAL(triggered()), this, SLOT(ToggleDockWidgetTitleBars()));
	//m_toolbar->action_ToggleSettings->setCheckable(true);
	//m_toolbar->action_ToggleSettings->setChecked(true);
	//connect(m_toolbar->action_ToggleSettings, SIGNAL(triggered()), this, SLOT(ToggleSettings()));
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolbar);
}

void iAFiAKErModuleInterface::ToggleDockWidgetTitleBars()
{
	iAUncertaintyAttachment* attach = GetAttachment<iAUncertaintyAttachment>();
	if (!attach)
	{
		DEBUG_LOG("Uncertainty exploration was not loaded properly!");
		return;
	}
	attach->ToggleDockWidgetTitleBars();
}

void iAFiAKErModuleInterface::ToggleSettings()
{
	iAUncertaintyAttachment* attach = GetAttachment<iAUncertaintyAttachment>();
	if (!attach)
	{
		DEBUG_LOG("Uncertainty exploration was not loaded properly!");
		return;
	}
	attach->ToggleSettings();
}
*/