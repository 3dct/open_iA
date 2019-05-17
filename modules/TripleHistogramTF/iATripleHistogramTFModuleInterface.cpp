/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <iATripleHistogramTFModuleInterface.h>

#include "tf_2mod/dlg_tf_2mod.h"
#include "tf_3mod/dlg_tf_3mod.h"

#include <mainwindow.h>
#include <mdichild.h>

#include <QMessageBox>

void iATripleHistogramTFModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuMultiModalChannel = getMenuWithTitle(toolsMenu, QString("Multi-Modal/-Channel Images"), false);

	QAction *action_2mod = new QAction(m_mainWnd);
	action_2mod->setText(QApplication::translate("MainWindow", "Double Histogram Transfer Function", 0));
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, action_2mod, true);
	connect(action_2mod, SIGNAL(triggered()), this, SLOT(menuItemSelected_2mod()));

	QAction *action_3mod = new QAction(m_mainWnd);
	action_3mod->setText(QApplication::translate("MainWindow", "Triple Histogram Transfer Function", 0));
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, action_3mod, true);
	connect(action_3mod, SIGNAL(triggered()), this, SLOT(menuItemSelected_3mod()));
}

void iATripleHistogramTFModuleInterface::menuItemSelected_2mod()
{
	PrepareActiveChild();
	m_tf_2mod = new dlg_tf_2mod(m_mdiChild);

	m_mdiChild->tabifyDockWidget(m_mdiChild->logs, m_tf_2mod);

	m_tf_2mod->show();
	m_tf_2mod->raise();
}

void iATripleHistogramTFModuleInterface::menuItemSelected_3mod()
{
	PrepareActiveChild();
	m_tf_3mod = new dlg_tf_3mod(m_mdiChild);

	//m_mdiChild->addDockWidget(Qt::BottomDockWidgetArea, thtf);
	m_mdiChild->tabifyDockWidget(m_mdiChild->logs, m_tf_3mod);

	m_tf_3mod->show();
	m_tf_3mod->raise();
}