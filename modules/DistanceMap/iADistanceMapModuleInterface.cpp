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
#include "iADistanceMapModuleInterface.h"

#include "dlg_commoninput.h"
#include "iADistanceMap.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QSettings>

void iADistanceMapModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuDistance_Map = getMenuWithTitle(filtersMenu, QApplication::translate("MainWindow", "Distance Map", 0));
	QAction * actionSigned_Maurer_Distance_Map = new QAction(QApplication::translate("MainWindow", "Signed Maurer Distance Map", 0), m_mainWnd );
	QAction * actionDanielsson_Distance_Map = new QAction( QApplication::translate( "MainWindow", "Danielsson Distance Map", 0 ), m_mainWnd );
	menuDistance_Map->addAction(actionSigned_Maurer_Distance_Map);
	menuDistance_Map->addAction( actionDanielsson_Distance_Map );
	connect( actionSigned_Maurer_Distance_Map, SIGNAL( triggered() ), this, SLOT( signed_maurer_distance_map() ) );
	connect( actionDanielsson_Distance_Map, SIGNAL( triggered() ), this, SLOT( danielsson_distance_map() ) );
}

void iADistanceMapModuleInterface::signed_maurer_distance_map()
{
	//set parameters
	QSettings settings;
	smdmi = settings.value( "Filters/DistanceMap/SignedMaurer/smdmi" ).toBool();
	smdms = settings.value( "Filters/DistanceMap/SignedMaurer/smdms" ).toBool();
	smdmp = settings.value( "Filters/DistanceMap/SignedMaurer/smdmp" ).toBool();
	smdmn = settings.value( "Filters/DistanceMap/SignedMaurer/smdmn" ).toBool();

	QStringList inList = (QStringList() << tr( "$Image spacing" ) << tr( "$Squared distance" ) << tr( "$Inside positive" ) << tr( "$Remove negative values" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( smdmi ) << tr( "%1" ).arg( smdms ) << tr( "%1" ).arg( smdmp ) << tr( "%1" ).arg( smdmn );
	dlg_commoninput dlg( m_mainWnd, "Signed Maurer Distance Map", 4, inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;
	smdmi = dlg.getCheckValues()[0]; 
	smdms = dlg.getCheckValues()[1]; 
	smdmp = dlg.getCheckValues()[2]; 
	smdmn = dlg.getCheckValues()[3];

	settings.setValue( "Filters/DistanceMap/SignedMaurer/smdmi", smdmi );
	settings.setValue( "Filters/DistanceMap/SignedMaurer/smdms", smdms );
	settings.setValue( "Filters/DistanceMap/SignedMaurer/smdmp", smdmp );
	settings.setValue( "Filters/DistanceMap/SignedMaurer/smdmn", smdmn );

	//prepare
	QString filterName = "Signed Maurer Distance Map";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iADistanceMap* thread = new iADistanceMap( filterName, SIGNED_MAURER_DISTANCE_MAP,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setSMDMParameters( smdmi, smdms, smdmp, smdmn );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iADistanceMapModuleInterface::danielsson_distance_map()
{
	//prepare
	QString filterName = tr( "Danielsson Distance Map" );
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iADistanceMap* thread = new iADistanceMap( filterName, DANIELSSON_DISTANCE_MAP,
											   m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}
