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
#include "iAEdgeDetectionModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAEdgeDetectionFilters.h"
#include "mainwindow.h"
#include "mdichild.h"

void iAEdgeDetectionModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuEdge_detection =getMenuWithTitle(filtersMenu, QApplication::translate("MainWindow", "Edge detection", 0));
	QAction * actionCanny = new QAction(QApplication::translate("MainWindow", "Canny", 0), m_mainWnd );
	menuEdge_detection->addAction(actionCanny);
	connect( actionCanny, SIGNAL( triggered() ), this, SLOT( canny_Edge_Detection() ) );
}

void iAEdgeDetectionModuleInterface::canny_Edge_Detection()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Variance" ) << tr( "#Maximum error" ) << tr( "#Upper" ) << tr( "#Lower" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( cedfVariance ) << tr( "%1" ).arg( cedfMaximumError ) << tr( "%1" ).arg( cedfUpper ) << tr( "%1" ).arg( cedfLower );
	dlg_commoninput dlg( m_mainWnd, "Canny Edge Detection", inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;
	cedfVariance = dlg.getDblValue(0);
	cedfMaximumError = dlg.getDblValue(1);
	cedfUpper = dlg.getDblValue(2);
	cedfLower = dlg.getDblValue(3);
	//prepare
	QString filterName = "Canny edge detection";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAEdgeDetectionFilters* thread = new iAEdgeDetectionFilters( filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setCEDParameters( cedfVariance, cedfMaximumError, cedfUpper, cedfLower );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

