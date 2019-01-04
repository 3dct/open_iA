/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iATripleHistogramTFModuleInterface.h"

#include <dlg_TripleHistogramTF.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <QMessageBox>

void iATripleHistogramTFModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	QMenu * filtersMenu = m_mainWnd->getToolsMenu();  // alternatively, you can use getToolsMenu() here if you want to add a tool
	QAction * actionTest = new QAction( m_mainWnd );
	actionTest->setText( QApplication::translate( "MainWindow", "Triple Histogram Transfer Function", 0 ) );
	AddActionToMenuAlphabeticallySorted(filtersMenu,  actionTest, true ); // "By specifying false in the third parameter to AddActionToMenuAlphabeticallySorted we say that the menu entry should not depend on the availability of an open file (if you say true here, the menu entry will only be enabled if a file is open)"
	connect( actionTest, SIGNAL( triggered() ), this, SLOT(MenuItemSelected() ) ); // "The added menu entry is linked to the method TestAction via the call to the connect method (inherited from QObject). In the TestAction we just open a simple information message box"
}

void iATripleHistogramTFModuleInterface::MenuItemSelected()
{
	PrepareActiveChild();
	thtf = new dlg_TripleHistogramTF(m_mdiChild);

	//m_mdiChild->addDockWidget(Qt::BottomDockWidgetArea, thtf);
	m_mdiChild->tabifyDockWidget(m_mdiChild->logs, thtf);

	thtf->show();
	thtf->raise();
}