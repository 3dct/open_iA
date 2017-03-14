/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "iAIntensityModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAIntensity.h"
#include "iASubtractImageFilter.h"
#include "mainwindow.h"
#include "mdichild.h"

void iAIntensityModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuIntensity = getMenuWithTitle(filtersMenu, QString( "Intensity" ) );
	QAction * actionDifference = new QAction(QApplication::translate("MainWindow", "Difference", 0), m_mainWnd );
	QAction * actionSubtractImage_Filter = new QAction(QApplication::translate("MainWindow", "SubtractImages Filter", 0), m_mainWnd );
	QAction * actionInvertIntensity = new QAction(QApplication::translate("MainWindow", "InvertIntensity", 0), m_mainWnd );
	QAction * actionMaskImage = new QAction(QApplication::translate("MainWindow", "MaskImage", 0), m_mainWnd );
	QAction * actionIntensityWindowing = new QAction(QApplication::translate("MainWindow", "Intensity Windowing", 0), m_mainWnd );
	menuIntensity->addAction( actionDifference );
	menuIntensity->addAction( actionSubtractImage_Filter );
	menuIntensity->addAction( actionInvertIntensity );
	menuIntensity->addAction( actionMaskImage );
	menuIntensity->addAction( actionIntensityWindowing );
	connect( actionDifference, SIGNAL( triggered() ), this, SLOT( difference_Image_Filter() ) );
	connect( actionSubtractImage_Filter, SIGNAL( triggered() ), this, SLOT( subtractimage_Filter() ) );
	connect( actionInvertIntensity, SIGNAL( triggered() ), this, SLOT( invert_intensity() ) );
	connect( actionMaskImage, SIGNAL( triggered() ), this, SLOT( mask() ) );
	connect( actionIntensityWindowing, SIGNAL( triggered() ), this, SLOT( intensity_windowing() ) );
}

void iAIntensityModuleInterface::difference_Image_Filter()
{
	//set parameters
	QStringList inList = (QStringList()
		<< tr( "#Difference threshold" ) << tr( "#Tolerance radius" ));
	QList<QVariant> inPara;
	inPara << tr( "%1" ).arg( difDifferenceThreshold ) << tr( "%1" ).arg( difToleranceRadius );

	dlg_commoninput dlg( m_mainWnd, "Difference Image Filter", 2, inList, inPara, NULL );

	if( dlg.exec() != QDialog::Accepted )
		return;

	difDifferenceThreshold = dlg.getValues()[0];
	difToleranceRadius = dlg.getValues()[1];

	MdiChild *child2 = GetSecondNonActiveChild();
	if (!child2)
	{
		return;
	}
	//prepare
	QString filterName = "Difference Image between " + child2->windowTitle() + " and";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity * thread = new iAIntensity( filterName, DIFFERENCE_IMAGE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setDIFParameters( difDifferenceThreshold, difToleranceRadius, child2->getImageData() );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::subtractimage_Filter()
{
	//set parameters
	MdiChild *child2 = GetSecondNonActiveChild();
	if (!child2)
	{
		return;
	}
	//prepare
	QString filterName = "Subtract Image " + child2->windowTitle() + " from";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iASubtractImageFilter* thread = new iASubtractImageFilter( filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setSIParameters( child2->getImageData() );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::invert_intensity()
{
	//prepare
	QString filterName = "Invert Intensity";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity* thread = new iAIntensity( filterName, INVERT_INTENSITY,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::mask()
{
	MdiChild *child2 = GetSecondNonActiveChild();
	if ( !child2 )
	{
		return;
	}
	//prepare
	QString filterName = "Mask Image " + child2->windowTitle();
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity * thread = new iAIntensity( filterName, MASK_IMAGE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setDIFParameters( 0, 0, child2->getImageData() );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAIntensityModuleInterface::intensity_windowing()
{
	//set parameters
	QStringList inList = ( QStringList()
						   << tr( "#WindowMinimum" ) << tr( "#WindowMaximum" ) << tr( "#OutputMinimum" ) << tr( "#OutputMaximum" ) );
	QList<QVariant> inPara;
	inPara << tr( "%1" ).arg( windowMinimum ) << tr( "%1" ).arg( windowMaximum ) << tr( "%1" ).arg( outputMinimum ) << tr( "%1" ).arg( outputMaximum );

	dlg_commoninput dlg( m_mainWnd, "Intensity Windowing Image Filter", 4, inList, inPara, NULL );

	if ( dlg.exec() != QDialog::Accepted )
		return;

	windowMinimum = dlg.getValues()[0];
	windowMaximum = dlg.getValues()[1];
	outputMinimum = dlg.getValues()[2];
	outputMaximum = dlg.getValues()[3];

	//prepare
	QString filterName = "Intensity Windowing";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAIntensity * thread = new iAIntensity( filterName, INTENSITY_WINDOWING,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setWIIFParameters( windowMinimum, windowMaximum, outputMinimum, outputMaximum );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}
