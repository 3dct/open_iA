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
#include "iAGeometricTransformationsModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAGeometricTransformations.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkImageData.h>

void iAGeometricTransformationsModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuGeometric_Transformations = getMenuWithTitle(filtersMenu, QApplication::translate("MainWindow", "Geometric Transformations", 0));
	
	QAction * actionResampler = new QAction(QApplication::translate("MainWindow", "Resampler", 0), m_mainWnd );
	menuGeometric_Transformations->addAction(actionResampler);
	connect( actionResampler, SIGNAL( triggered() ), this, SLOT( resampler() ) );

	QAction * actionExtract_Image = new QAction(QApplication::translate("MainWindow", "Extract Image", 0), m_mainWnd);
	menuGeometric_Transformations->addAction(actionExtract_Image);
	connect( actionExtract_Image, SIGNAL( triggered() ), this, SLOT( extractImage() ) );
}

void iAGeometricTransformationsModuleInterface::resampler()
{
	PrepareActiveChild();
	if (!m_mdiChild)
		return;
	QStringList inList = (QStringList() 
		<< tr("#OriginX") << tr("#OriginY") << tr("#OriginZ")
		<< tr("#SpacingX") << tr("#SpacingY") << tr("#SpacingZ")
		<< tr("#SizeX") << tr("#SizeY") << tr("#SizeZ" ))
		<< tr("+Interpolator");

	QStringList interpolators;
	interpolators
		<< QString("!")+iAGeometricTransformations::InterpLinear
		<< iAGeometricTransformations::InterpNearestNeighbour
		<< iAGeometricTransformations::InterpBSpline
		<< iAGeometricTransformations::InterpWindowedSinc
	;
	QList<QVariant> inPara;
	inPara
		<< tr( "%1" ).arg( rOriginX ) << tr( "%1" ).arg( rOriginY ) << tr( "%1" ).arg( rOriginZ )
		<< tr( "%1" ).arg( m_childData.imgData->GetSpacing()[1] )
		<< tr( "%1" ).arg( m_childData.imgData->GetSpacing()[0] )
		<< tr( "%1" ).arg( m_childData.imgData->GetSpacing()[2] )
		<< tr( "%1" ).arg( m_childData.imgData->GetExtent()[1] )
		<< tr( "%1" ).arg( m_childData.imgData->GetExtent()[3] )
		<< tr( "%1" ).arg( m_childData.imgData->GetExtent()[5] )
		<< interpolators;


	dlg_commoninput dlg( m_mainWnd, "Resampler", inList, inPara, NULL );

	if (dlg.exec() != QDialog::Accepted)
		return;
	rOriginX = dlg.getValues()[0];
	rOriginY = dlg.getValues()[1];
	rOriginZ = dlg.getValues()[2];
	rSpacingX = dlg.getValues()[3];
	rSpacingY = dlg.getValues()[4];
	rSpacingZ = dlg.getValues()[5];
	rSizeX = dlg.getValues()[6];
	rSizeY = dlg.getValues()[7];
	rSizeZ = dlg.getValues()[8];
	rInterpolator = dlg.getComboBoxValue(9);

	//prepare
	QString filterName = "Resampled";
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	rSizeX++; rSizeY++; rSizeZ++;
	iAGeometricTransformations* thread = new iAGeometricTransformations( filterName, RESAMPLER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setRParameters(
		rOriginX, rOriginY, rOriginZ,
		rSpacingX, rSpacingY, rSpacingZ,
		rSizeX, rSizeY, rSizeZ,
		rInterpolator
	);
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAGeometricTransformationsModuleInterface::extractImage()
{
	PrepareActiveChild();
	if (!m_mdiChild)
		return;
	MdiChild* origChild = m_mdiChild;
	m_childClosed = false;
	connect(origChild, SIGNAL(closed()), this, SLOT(childClosed()));
	QStringList inList = (QStringList() << tr( "*IndexX" ) << tr( "*IndexY" ) << tr( "*IndexZ" )
		<< tr( "*SizeX" ) << tr( "*SizeY" ) << tr( "*SizeZ" ) );
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( eiIndexX ) << tr( "%1" ).arg( eiIndexY ) << tr( "%1" ).arg( eiIndexZ )
		<< tr( "%1" ).arg( m_childData.imgData->GetExtent()[1] + 1 )
		<< tr( "%1" ).arg( m_childData.imgData->GetExtent()[3] + 1 )
		<< tr( "%1" ).arg( m_childData.imgData->GetExtent()[5] + 1 );

	dlg_commoninput dlg( m_mainWnd, "Extract Image", inList, inPara, NULL );
	dlg.connectMdiChild(origChild);
	dlg.setModal( false );
	dlg.show();
	origChild->activate( MdiChild::cs_ROI );
	origChild->setROI( eiIndexX, eiIndexY, eiIndexZ,
		m_childData.imgData->GetExtent()[1]+1,
		m_childData.imgData->GetExtent()[3]+1,
		m_childData.imgData->GetExtent()[5]+1);
	origChild->showROI();
	int result = dlg.exec();
	if (!m_mainWnd->isVisible() || m_childClosed)	// main window  or mdi child was closed in the meantime
		return;
	origChild->hideROI();
	origChild->deactivate();
	if (result != QDialog::Accepted)
		return;
	eiIndexX = dlg.getSpinBoxValue(0);
	eiIndexY = dlg.getSpinBoxValue(1);
	eiIndexZ = dlg.getSpinBoxValue(2);
	eiSizeX  = dlg.getSpinBoxValue(3);
	eiSizeY  = dlg.getSpinBoxValue(4);
	eiSizeZ  = dlg.getSpinBoxValue(5);
	//prepare
	QString filterName = "Extracted";
	// at the moment, PrepareResultChild always takes the active child, but that might have changed
	m_mdiChild = m_mainWnd->GetResultChild(origChild, filterName+" "+origChild->windowTitle());
	if (!m_mdiChild)
	{
		m_mainWnd->statusBar()->showMessage("Cannot get result child from main window!", 5000);
		return;
	}
	m_mdiChild->addStatusMsg( filterName );
	UpdateChildData();
	//execute
	m_mdiChild->setUpdateSliceIndicator( true );
	iAGeometricTransformations* thread = new iAGeometricTransformations( filterName, EXTRACT_IMAGE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setEParameters( eiIndexX, eiIndexY, eiIndexZ, eiSizeX, eiSizeY, eiSizeZ );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAGeometricTransformationsModuleInterface::childClosed()
{
	m_childClosed = true;
}
