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
#include "iA4DCTModuleInterface.h"
// iA
#include "dlg_commoninput.h"
#include "iA4DCTMainWin.h"
#include "iA4DCTSettings.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iASlicer.h"
#include "iASlicerWidget.h"
#include "mainwindow.h"
#include "mdichild.h"
#include "iAFeatureExtraction.h"
#include "iAFeatureExtractionDialog.h"
#include "iADefectClassifier.h"
#include "iAClassifyDefectsDialog.h"
// vtk
#include <vtkMath.h>
// itk
#include <itkConvolutionImageFilter.h>
#include <itkEllipseSpatialObject.h>
#include <itkImageFileWriter.h>
#include <itkImageKernelOperator.h>
#include <itkImageToVTKImageFilter.h>
#include <itkLabelGeometryImageFilter.h>
#include <itkNormalizedCorrelationImageFilter.h>
#include <itkSpatialObjectToImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkVTKImageToImageFilter.h>
// Qt
#include <QColor>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QObject>
#include <QSettings>
// std
#include <limits>

#define RAD_TO_DEG 57.295779513082320876798154814105

iA4DCTModuleInterface::iA4DCTModuleInterface( )
{ /* not implemented */ }

iA4DCTModuleInterface::~iA4DCTModuleInterface( )
{ /* not implemented */ }

void iA4DCTModuleInterface::Initialize( )
{
	if (!m_mainWnd)
		return;
	QMenu* toolsMenu = m_mainWnd->getToolsMenu( );

	// ToFix: the menu should be added through the standard way of adding modules menus.
	// But a new menu won't be enabled by the default till a mdichild is created or opened.
	// This hack allows to be the menu enabled by the default.
	QMenu* menu4DCT = new QMenu( toolsMenu );
	menu4DCT->setTitle( tr( "4DCT" ) );
	toolsMenu->addMenu( menu4DCT );

	QAction * newProj = new QAction( m_mainWnd );
	newProj->setText( QApplication::translate( "MainWindows", "New 4DCT project", 0 ) );
	newProj->setShortcut( QKeySequence( Qt::ALT + Qt::Key_4, Qt::Key_N ) );
	connect( newProj, SIGNAL( triggered( ) ), this, SLOT( newProj( ) ) );
	menu4DCT->addAction( newProj );

	QAction * openProj = new QAction( m_mainWnd );
	openProj->setText( QApplication::translate( "MainWindows", "Open 4DCT project", 0 ) );
	openProj->setShortcut( QKeySequence( Qt::ALT + Qt::Key_4, Qt::Key_O ) );
	connect( openProj, SIGNAL( triggered( ) ), this, SLOT( openProj( ) ) );
	menu4DCT->addAction( openProj );

	QAction* saveProj = new QAction( m_mainWnd );
	saveProj->setText( QApplication::translate( "MainWindows", "Save 4DCT project", 0 ) );
	saveProj->setShortcut( QKeySequence( Qt::ALT + Qt::Key_4, Qt::Key_S ) );
	connect( saveProj, SIGNAL( triggered( ) ), this, SLOT( saveProj( ) ) );
	menu4DCT->addAction( saveProj );

	QAction* featureExtraction = new QAction( m_mainWnd );
	featureExtraction->setText( QApplication::translate( "MainWindows", "Extract features to file", 0 ) );
	connect( featureExtraction, SIGNAL( triggered( ) ), this, SLOT( extractFeaturesToFile( ) ) );
	menu4DCT->addAction( featureExtraction );

	QAction* defectClassification = new QAction( m_mainWnd );
	defectClassification->setText( QApplication::translate( "MainWindows", "Defect classification", 0 ) );
	connect( defectClassification, SIGNAL( triggered( ) ), this, SLOT( defectClassification( ) ) );
	menu4DCT->addAction( defectClassification );
}

/*============

	Slots

============*/

void iA4DCTModuleInterface::openProj( )
{
	QSettings settings;
	QString fileName = QFileDialog::getOpenFileName(
		m_mainWnd,
		tr( "Open 4DCT proj" ),
		settings.value( S_4DCT_OPEN_DIR ).toString( ),
		tr( "4DCT project (*.xml)" ) );

	QFileInfo file( fileName );
	if( !file.exists( ) ) {
		return;
	}
	settings.setValue( S_4DCT_OPEN_DIR, file.absolutePath( ) );

	iA4DCTMainWin* sv = new iA4DCTMainWin( m_mainWnd );
	sv->load( fileName );
	m_mainWnd->mdiArea->addSubWindow( sv );
	sv->show( );
}

void iA4DCTModuleInterface::newProj( )
{
	iA4DCTMainWin* sv = new iA4DCTMainWin( m_mainWnd );
	m_mainWnd->mdiArea->addSubWindow( sv );
	sv->show( );
}

void iA4DCTModuleInterface::saveProj( )
{
	QMdiSubWindow* subWnd = m_mainWnd->mdiArea->currentSubWindow( );
	iA4DCTMainWin* stackView = qobject_cast<iA4DCTMainWin*>( subWnd->widget( ) );
	if( stackView != NULL ) {
		stackView->save( );
	}
}

void iA4DCTModuleInterface::extractFeaturesToFile( )
{
	iAFeatureExtractionDialog dialog;
	if( dialog.exec( ) == QDialog::Rejected )
	{
		return;
	}

	iAFeatureExtraction::run( dialog.getInputImg( ), dialog.getOutputFile( ) );
}

void iA4DCTModuleInterface::defectClassification()
{
	iAClassifyDefectsDialog dlg;
	if( dlg.exec( ) == QDialog::Rejected )
	{
		return;
	}

	iADefectClassifier df;
	iADefectClassifier::Parameters params;
	params.Spacing = dlg.ui.dsbSpacing->value( );
	params.ElongationP = dlg.ui.dsbElongationP->value( );
	params.ElongationD = dlg.ui.dsbElongationD->value( );
	params.LengthRangeP[0] = dlg.ui.dsbLengthRangeP_1->value( );
	params.LengthRangeP[1] = dlg.ui.dsbLengthRangeP_2->value( );
	params.WidthRangeP[0] = dlg.ui.dsbWidthRangeP_1->value( );
	params.WidthRangeP[1] = dlg.ui.dsbWidthRangeP_2->value( );
	params.AngleP = dlg.ui.dsbAngleP->value( );
	params.AngleB = dlg.ui.dsbAngleB->value( );
	params.AngleD = dlg.ui.dsbAngleD->value( );
	params.NeighborhoodDistP = dlg.ui.dsbNeighborhoodDistanceP->value( );
	params.NeighborhoodDistFF = dlg.ui.dsbNeighborhoodDistanceFF->value( );
	params.BigVolumeThreshold = dlg.ui.dsbBigVolumeThreshold->value( );
	params.FibersFile = dlg.ui.Fibers->ui.Path->text( ).toStdString( );
	params.FeaturesFile = dlg.ui.Defects->ui.Path->text( ).toStdString( );
	params.OutputDir = dlg.ui.Output->ui.Path->text( ).toStdString( );

	df.run( params );
}

//void iA4DCTModuleInterface::enableDensityMap()
//{
//	PrepareActiveChild();
//	/*m_densityMap = new dlg_densityMap(m_mainWnd, m_mdiChild);
//	m_mdiChild->tabifyDockWidget(m_childData.logs, m_densityMap);*/
//
//	dlg_4dctRegistration* reg = new dlg_4dctRegistration();
//	m_mainWnd->addSubWindow(reg);
//	reg->show();
//
//	QList<QMdiSubWindow*> list = m_mainWnd->MdiChildList();
//	foreach(QMdiSubWindow* window, m_mainWnd->MdiChildList())
//	{
//		MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
//		mdiChild->getSlicerXY()->widget()->set4DCTRegistration(reg);
//		mdiChild->getSlicerXZ()->widget()->set4DCTRegistration(reg);
//		mdiChild->getSlicerYZ()->widget()->set4DCTRegistration(reg);
//	}
//}
