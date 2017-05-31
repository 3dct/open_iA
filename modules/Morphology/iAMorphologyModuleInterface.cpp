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
#include "iAMorphologyModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAMorphologyFilters.h"
#include "mainwindow.h"
#include "mdichild.h"

void iAMorphologyModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menu_Morphology_Filters = getMenuWithTitle(filtersMenu, QString( "Morphology" ) );
	QAction * actionDilation_Filter = new QAction(QApplication::translate("MainWindow", "Dilation", 0), m_mainWnd );
	QAction * actionErosion_Filter = new QAction(QApplication::translate("MainWindow", "Erosion", 0), m_mainWnd );
	QAction * actionVesselEnhancement_Filter = new QAction(QApplication::translate("MainWindow", "Vessel Enhancement", 0), m_mainWnd );

	menu_Morphology_Filters->addAction( actionDilation_Filter );
	menu_Morphology_Filters->addAction( actionErosion_Filter );
	menu_Morphology_Filters->addAction( actionVesselEnhancement_Filter );

	connect( actionDilation_Filter, SIGNAL( triggered() ), this, SLOT( dilation_filter() ) );
	connect( actionErosion_Filter, SIGNAL( triggered() ), this, SLOT( erosion_filter() ) );
	connect( actionVesselEnhancement_Filter, SIGNAL( triggered() ), this, SLOT( vessel_enhancement_filter() ) );
}

void iAMorphologyModuleInterface::dilation_filter()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Structuring element radius" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( dilrad );

	dlg_commoninput dlg( m_mainWnd, "Dilation Filter", inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;
	
	dilrad = dlg.getValues()[0];
	//prepare
	QString filterName = "Dilated " + QString::number(dilrad);
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAMorphologyFilters* thread = new iAMorphologyFilters( filterName, DILATION_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setMORPHParameters( dilrad, NULL );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAMorphologyModuleInterface::erosion_filter()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Structuring element radius" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( errad );

	dlg_commoninput dlg( m_mainWnd, "Erosion Filter", inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;
	
	errad = dlg.getValues()[0];

	//prepare
	QString filterName = "Eroded " + QString::number(errad);
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );

	//execute
	iAMorphologyFilters* thread = new iAMorphologyFilters( filterName, EROSION_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setMORPHParameters( errad, NULL );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAMorphologyModuleInterface::vessel_enhancement_filter()
{
	//set parameters
	QStringList inList = (QStringList() << tr( "#Sigma" ));
	QList<QVariant> inPara; inPara << tr( "%1" ).arg( sigmaenh );

	dlg_commoninput dlg( m_mainWnd, "Enhancement Filter", inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;
	
	sigmaenh = dlg.getValues()[0];
	//prepare
	QString filterName = "Vessel Enhancement";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAMorphologyFilters* thread = new iAMorphologyFilters( filterName, VESSEL_ENHANCEMENT_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setMORPHParameters( sigmaenh, NULL );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}
