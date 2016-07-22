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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iA4DCTVisWin.h"
// iA
#include "dlg_4DCTFileOpen.h"
#include "dlg_regionView.h"
#include "iA4DCTAllVisualizationsDockWidget.h"
#include "iA4DCTCurrentVisualizationsDockWidget.h"
#include "iA4DCTData.h"
#include "iA4DCTFractureVisDockWidget.h"
#include "iA4DCTMainWin.h"
#include "iA4DCTPlaneDockWidget.h"
#include "iA4DCTRegionViewDockWidget.h"
#include "iA4DCTSettings.h"
#include "iA4DCTSettingsDockWidget.h"
#include "iABoundingBoxVisModule.h"
#include "iAFractureVisModule.h"
#include "iAMhdFileInfo.h"
#include "iAPlaneVisModule.h"
#include "iARegionVisModule.h"
#include "iAVisModule.h"
#include "iAVisModuleItem.h"
#include "iAMagicLens.h"
#include "iA4DCTToolsDockWidget.h"
#include "DensityMap.h"
// Qt
#include <QFileDialog>
#include <QSettings>
#include <QString>
#include <QVector>
// itk
#include <itkBinaryThresholdImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkResampleImageFilter.h>
// vtk
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>

iA4DCTVisWin::iA4DCTVisWin( iA4DCTMainWin* parent /*= 0*/ )
	: QMainWindow( parent )
	, m_mainWin( parent )
	, m_currentStage( 0 )
	, m_isVirgin( true )
{
	setupUi( this );
	// default size
	m_size[ 0 ] = 100; m_size[ 1 ] = 100; m_size[ 2 ] = 100;
	m_timer.setInterval( 1000 ); // 1 sec

	// GUI
	splitter->setStretchFactor( 0, 1 );
	splitter->setStretchFactor( 1, 0 );

	// setup renderer
	m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	qvtkWidget->SetMainRenderWindow(m_renderWindow);
	m_mainRen = vtkSmartPointer<vtkRenderer>::New();
	m_mainRen->SetLayer(0);
	m_mainRen->SetBackground(0.5, 0.5, 0.5);
	m_mainRen->InteractiveOn();
	qvtkWidget->GetRenderWindow()->AddRenderer(m_mainRen);
	m_magicLensRen = qvtkWidget->getLensRenderer();

	// setup dock widgets
	// tools
	m_dwTools = new iA4DCTToolsDockWidget( this );
	// settings
	m_dwSettings = new iA4DCTSettingsDockWidget( this );
	m_dwSettings->setRenderer( m_mainRen );
	connect( m_dwSettings, SIGNAL( updateRenderWindow() ), this, SLOT( updateRenderWindow() ) );
	// fracture vis
	m_dwFractureVis = new iA4DCTFractureVisDockWidget( this );
	m_dwFractureVis->setData( m_mainWin->getStageData() );
	connect( m_dwFractureVis, SIGNAL( updateRenderWindow() ), this, SLOT( updateRenderWindow() ) );
	// plane
	m_dwPlane = new iA4DCTPlaneDockWidget( this );
	connect( m_dwPlane, SIGNAL( updateRenderWindow() ), this, SLOT( updateRenderWindow() ) );
	// all visualizations view
	m_dwAllVis = new iA4DCTAllVisualizationsDockWidget( this );
	m_dwAllVis->setCollection( &m_visModules );
	connect( m_dwAllVis, SIGNAL( addedVisualization() ), this, SLOT( addedVisualization() ) );
	// current visualizations
	m_dwCurrentVis = new iA4DCTCurrentVisualizationsDockWidget( this );
	m_dwCurrentVis->setCollection( &m_visModules );
	connect( m_dwCurrentVis, SIGNAL( selectedVisModule( iAVisModule * ) ), this, SLOT( selectedVisModule( iAVisModule * ) ) );
	connect( m_dwCurrentVis, SIGNAL( removedVisModule() ), this, SLOT( updateVisualizations() ) );
	// region visualization
	m_dwRegionVis = new iA4DCTRegionViewDockWidget( this );
	connect( m_dwRegionVis, SIGNAL( updateRenderWindow() ), this, SLOT( updateRenderWindow() ) );

	// setup dock widget layout
	setTabPosition( Qt::LeftDockWidgetArea, QTabWidget::North );
	setTabPosition( Qt::RightDockWidgetArea, QTabWidget::North );
	addDockWidget( Qt::RightDockWidgetArea, m_dwAllVis );
	addDockWidget( Qt::RightDockWidgetArea, m_dwCurrentVis );
	addDockWidget( Qt::RightDockWidgetArea, m_dwTools );
	addDockWidget( Qt::LeftDockWidgetArea, m_dwSettings );
	addDockWidget( Qt::LeftDockWidgetArea, m_dwFractureVis );
	addDockWidget( Qt::LeftDockWidgetArea, m_dwPlane );
	addDockWidget( Qt::LeftDockWidgetArea, m_dwRegionVis );

	setEnabledToolsDockWidgets( false );

	// setup signals
	connect( m_dwTools->pbDefectViewerAdd,		SIGNAL( clicked( ) ), this, SLOT( addDefectView( ) ) );
	connect( m_dwTools->pbBoundingBoxAdd,		SIGNAL( clicked( ) ), this, SLOT( addBoundingBox( ) ) );
	//connect( m_dwTools->pbDensityMapAdd,		SIGNAL( clicked( ) ), this, SLOT(  ) );
	connect( m_dwTools->pbFractureViewerAdd,	SIGNAL( clicked( ) ), this, SLOT( onExtractButtonClicked( ) ) );
	connect( m_dwTools->pbFractureViewerLoad,	SIGNAL( clicked( ) ), this, SLOT( onLoadButtonClicked( ) ) );
	connect( m_dwTools->pbSurfaceViewerAdd,		SIGNAL( clicked( ) ), this, SLOT( addSurfaceVis( ) ) );
	connect( m_dwTools->pbCalcDensityMap,		SIGNAL( clicked( ) ), this, SLOT( calcDensityMap( ) ) );
	connect( actionMagicLens,	SIGNAL( toggled( bool ) ), this, SLOT( enableMagicLens( bool ) ) );
	connect( sStage,	SIGNAL( valueChanged( int ) ), this, SLOT( onStageSliderValueChanged( int ) ) );
	connect( pbFirst,		SIGNAL( clicked() ), this, SLOT( onFirstButtonClicked() ) );
	connect( pbPrevious,	SIGNAL( clicked() ), this, SLOT( onPreviousButtonClicked() ) );
	connect( pbNext,		SIGNAL( clicked() ), this, SLOT( onNextButtonClicked() ) );
	connect( pbLast,		SIGNAL( clicked() ), this, SLOT( onLastButtonClicked() ) );
	connect( pbPlay,		SIGNAL( clicked( bool ) ), this, SLOT( onPlayButtonClicked( bool ) ) );
	connect( &m_timer,	SIGNAL( timeout() ), this, SLOT( onNextButtonClicked() ) );
	connect( sbInterval,	SIGNAL( valueChanged( int ) ), this, SLOT( onIntervalValueChanged( int ) ) );
	connect( actionResetCam,	SIGNAL( triggered( ) ), this, SLOT( resetCamera( ) ) );
	connect( actionXYView,		SIGNAL( triggered( ) ), this, SLOT( setXYView( ) ) );
	connect( actionXZView,		SIGNAL( triggered( ) ), this, SLOT( setXZView( ) ) );
	connect( actionYZView,		SIGNAL( triggered( ) ), this, SLOT( setYZView( ) ) );
	connect( actionXYBackView,	SIGNAL( triggered( ) ), this, SLOT( setXYBackView( ) ) );
	connect( actionXZBackView,	SIGNAL( triggered( ) ), this, SLOT( setXZBackView( ) ) );
	connect( actionYZBackView,	SIGNAL( triggered( ) ), this, SLOT( setYZBackView( ) ) );
}

iA4DCTVisWin::~iA4DCTVisWin()
{ /* not implemented */ }

void iA4DCTVisWin::setImageSize( double * size )
{
	m_size[ 0 ] = size[ 0 ]; m_size[ 1 ] = size[ 1 ]; m_size[ 2 ] = size[ 2 ];
	const double dNear = 0.1;
	double dFar = std::max( m_size[0], std::max( m_size[1], m_size[2] ) ) * 8;
}

void iA4DCTVisWin::setNumberOfStages( int number )
{
	sStage->setMaximum( number - 1 );
}

void iA4DCTVisWin::updateRenderWindow()
{
	qvtkWidget->GetRenderWindow()->Render();
}

// show dialog to select an image file
bool iA4DCTVisWin::showDialog( QString & imagePath )
{
	/*int stageId;
	return showDialog( imagePath, stageId );*/

	dlg_4DCTFileOpen dialog( this );
	dialog.setData( m_mainWin->getStageData() );
	if( dialog.exec() != QDialog::Accepted )
		return false;
	imagePath = dialog.getFile().Path;
	return true;
}

//// save current surface to a file
//void iA4DCTVisWin::onSaveButtonClicked()
//{
//	// save heigthmap
//	QSettings settings;
//	QString fileName = QFileDialog::getSaveFileName( this, tr( "Save surface" ),
//		settings.value( S_4DCT_SAVE_SURFACE_DIR ).toString(), tr( "Images (*.mhd)" ) );
//	if(fileName.isEmpty())	{
//		return;
//	}
//	settings.setValue( S_4DCT_SAVE_SURFACE_DIR, fileName );
//
//	/*typedef itk::ImageFileWriter<MapType> WriterType;
//	WriterType::Pointer writer = WriterType::New();
//	writer->SetInput(m_heightmap);
//	writer->SetFileName(fileName.toStdString());
//	writer->Update();*/
//}

// load a surface from the input surface file
void iA4DCTVisWin::onLoadButtonClicked()
{
	QString imagePath;
	if(!showDialog( imagePath )) {
		return;
	}

	iAFractureVisModule* fractureView = new iAFractureVisModule;
	fractureView->attachTo( m_mainRen );
	fractureView->setSize( m_size );
	fractureView->load( imagePath );
	static int number = 0;
	m_visModules.addModule( fractureView, "Fracture (loaded) " + QString::number( number++ ) );
	m_dwAllVis->updateContext();
}

// calculate a new surface from the input file
void iA4DCTVisWin::onExtractButtonClicked()
{
	QString imagePath;
	if(!showDialog( imagePath )) {
		return;
	}
	iAMhdFileInfo mhdFileInfo( imagePath );
	double imgSize[ 3 ], imgSpacing[ 3 ];
	mhdFileInfo.getFileDimSize( imgSize );
	mhdFileInfo.getElementSpacing( imgSpacing );
	double size[ 3 ] = { imgSize[ 0 ] * imgSpacing[ 0 ], imgSize[ 1 ] * imgSpacing[ 1 ], imgSize[ 2 ] * imgSpacing[ 2 ] };
	//extract(imagePath);

	iAFractureVisModule* fractureView = new iAFractureVisModule;
	if( actionAddToMagicLens->isChecked() )
	{
		fractureView->attachTo( m_magicLensRen );
	}
	else 
	{
		fractureView->attachTo( m_mainRen );
	}
	fractureView->setSize( size );
	fractureView->extract( imagePath );
	static int number = 0;
	m_visModules.addModule( fractureView, "Fracture (extracted) " + QString::number( number++ ) );

	double bounds[6];
	fractureView->getBounds( bounds );
	iABoundingBoxVisModule * boundingBox = new iABoundingBoxVisModule;
	double bbSize[3] = { (bounds[1] - bounds[0]) / SCENE_SCALE, (bounds[3] - bounds[2]) / SCENE_SCALE, (bounds[5] - bounds[4]) / SCENE_SCALE };
	boundingBox->setSize( bbSize );
	
	if( actionAddToMagicLens->isChecked() )
	{
		boundingBox->attachTo( m_magicLensRen );
	}
	else 
	{
		boundingBox->attachTo( m_mainRen );
	}
	boundingBox->setPosition( bounds[0], bounds[2], bounds[4] );
	m_visModules.addModule( boundingBox, "Fracture bounding box " + QString::number( number++ ) );

	m_dwAllVis->updateContext();
}

void iA4DCTVisWin::addSurfaceVis()
{
	dlg_regionView dialog( this );
	dialog.setData( m_mainWin->getStageData() );
	if( dialog.exec() != QDialog::Accepted )
		return;

	QString imagePath = dialog.getImagePath();
	double threshold = dialog.getThreshold();

	// add vis module
	iARegionVisModule * regionView = new iARegionVisModule;
	if( actionAddToMagicLens->isChecked() )
	{
		regionView->attachTo( m_magicLensRen );
	}
	else
	{
		regionView->attachTo( m_mainRen );
	}
	regionView->setDefectDensity( threshold );
	//regionView->setDensityMapDimension( dim );
	regionView->setImage( imagePath );
	static int number = 0;
	m_visModules.addModule( regionView, "Region vis " + QString::number( number++ ) );
	m_dwAllVis->updateContext();
}

void iA4DCTVisWin::onStageSliderValueChanged( int val )
{
	m_currentStage = val;
	m_dwAllVis->setCurrentStage( m_currentStage );
	m_dwCurrentVis->setCurrentStage( m_currentStage );

	updateVisualizations();
}

void iA4DCTVisWin::addBoundingBox()
{
	// add vis module
	iABoundingBoxVisModule * boundingBox = new iABoundingBoxVisModule;
	if( actionAddToMagicLens->isChecked() )
	{
		boundingBox->attachTo( m_magicLensRen );
	}
	else
	{
		boundingBox->attachTo( m_mainRen );
	}
	boundingBox->setSize( m_size );
	static int number = 0;
	m_visModules.addModule( boundingBox, "Bounding box " + QString::number( number++ ) );
	m_dwAllVis->updateContext();
}

void iA4DCTVisWin::addDefectView()
{
	// open dialog
	QString imagePath;
	if( !showDialog( imagePath ) ) {
		return;
	}

	// add vis module
	iAPlaneVisModule * planeVis = new iAPlaneVisModule;
	if( actionAddToMagicLens->isChecked() )
	{
		planeVis->attachTo( m_magicLensRen );
	}
	else
	{
		planeVis->attachTo( m_mainRen );
	}
	planeVis->setImage( imagePath );
	planeVis->setSize( m_size );
	static int number = 0;
	m_visModules.addModule( planeVis, "Slice " + QString::number( number++ ) );
	m_dwAllVis->updateContext();
}

void iA4DCTVisWin::updateVisualizations()
{
	QList<iAVisModuleItem *> modules = m_visModules.getModules();
	for(auto m : modules) {
		int index = m->stages.indexOf( m_currentStage );
		if(index != -1) {
			m->module->enable();
			if( m_isVirgin ) { m_isVirgin = false; m_mainRen->ResetCamera( ); }
		} else {
			m->module->disable();
		}
	}
	updateRenderWindow();
}

void iA4DCTVisWin::addedVisualization()
{
	updateVisualizations();
	m_dwCurrentVis->updateContext();
}

void iA4DCTVisWin::selectedVisModule( iAVisModule * visModule )
{
	iARegionVisModule * regionVis = dynamic_cast< iARegionVisModule * >( visModule );
	iABoundingBoxVisModule * boundingBox = dynamic_cast< iABoundingBoxVisModule * >( visModule );
	iAFractureVisModule * fractureVis = dynamic_cast< iAFractureVisModule * >( visModule );
	iAPlaneVisModule * planeVis = dynamic_cast< iAPlaneVisModule * >( visModule );

	setEnabledToolsDockWidgets( false );

	if(regionVis) {
		m_dwRegionVis->setEnabled( true );
		m_dwRegionVis->attachTo( regionVis );
	} else if(boundingBox) {
		m_dwSettings->setBoundingBox( boundingBox );
	} else if(fractureVis) {
		//m_dwColoring->setEnabled( true );
		m_dwFractureVis->setEnabled( true );
		m_dwFractureVis->attachTo( fractureVis );
	} else if(planeVis) {
		m_dwPlane->setEnabled( true );
		m_dwPlane->attachTo( planeVis );
	}
}

void iA4DCTVisWin::onFirstButtonClicked( )
{
	sStage->setValue( sStage->minimum() );
}

void iA4DCTVisWin::onPreviousButtonClicked( )
{
	if( sStage->value() == sStage->minimum( ) ) {
		onLastButtonClicked();
	} else {
		sStage->setValue( sStage->value() - 1 );
	}
}

void iA4DCTVisWin::onNextButtonClicked( )
{
	if(sStage->value() == sStage->maximum()) {
		onFirstButtonClicked();
	} else {
		sStage->setValue( sStage->value() + 1 );
	}
}

void iA4DCTVisWin::onLastButtonClicked( )
{
	sStage->setValue( sStage->maximum() );
}

void iA4DCTVisWin::onPlayButtonClicked( bool checked )
{
	if(checked) {
		m_timer.start();
	} else {
		m_timer.stop();
	}
}

void iA4DCTVisWin::onIntervalValueChanged( int val )
{
	m_timer.setInterval( val );
}

void iA4DCTVisWin::setEnabledToolsDockWidgets( bool enabled )
{
	m_dwFractureVis->setEnabled( enabled );
	m_dwPlane->setEnabled( enabled );
	m_dwRegionVis->setEnabled( enabled );
}

void iA4DCTVisWin::enableMagicLens( bool enable )
{
	if( enable )
	{
		qvtkWidget->magicLensOn( );
	}
	else
	{
		qvtkWidget->magicLensOff( );
	}
}

void iA4DCTVisWin::resetCamera( )
{
	m_mainRen->ResetCamera( );
	qvtkWidget->GetRenderWindow()->Render();
}

void iA4DCTVisWin::setXYView()
{
	m_mainRen->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera()->SetPosition( 0, 0, 1 );
	m_mainRen->GetActiveCamera()->SetViewUp( 0., -1., 0. );
	resetCamera( );
}

void iA4DCTVisWin::setXYBackView()
{
	m_mainRen->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera()->SetPosition( 0, 0, -1 );
	m_mainRen->GetActiveCamera()->SetViewUp( 0., -1., 0. );
	resetCamera( );
}

void iA4DCTVisWin::setXZView( )
{
	m_mainRen->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera()->SetPosition( 0, 1, 0 );
	m_mainRen->GetActiveCamera()->SetViewUp( 0, 0., 1. );
	resetCamera( );
}

void iA4DCTVisWin::setXZBackView( )
{
	m_mainRen->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera()->SetPosition( 0, -1., 0. );
	m_mainRen->GetActiveCamera()->SetViewUp( .0, 0., 1. );
	resetCamera( );
}

void iA4DCTVisWin::setYZView( )
{
	m_mainRen->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera()->SetPosition( 1, 0, 0 );
	m_mainRen->GetActiveCamera()->SetViewUp( 0., 0., 1. );
	resetCamera( );
}

void iA4DCTVisWin::setYZBackView( )
{
	m_mainRen->GetActiveCamera()->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera()->SetPosition( -1, 0, 0 );
	m_mainRen->GetActiveCamera()->SetViewUp( 0., 0., 1. );
	resetCamera( );
}

void iA4DCTVisWin::calcDensityMap()
{
	int size[3] = {30, 30, 30};
	//DensityMap::calculate("K:\\\\[transfer]\\\\GD301\\\\440\\\\GD301_woCA_3um_440N_mask.mhd", "K:\\\\[transfer]\\\\GD301\\\\440\\\\GD301_woCA_3um_440N_density_map.mhd", size);
	//DensityMap::calculate("K:\\\\[transfer]\\\\GD301\\\\422\\\\GD301_woCA_3um_422N_mask.mhd", "K:\\\\[transfer]\\\\GD301\\\\422\\\\GD301_woCA_3um_422N_density_map.mhd", size);
	//int size[3] = {16, 11, 18};
	DensityMap::calculate("K:\\[transfer]\\GD301\\374\\GD301_woCA_3um_374N_mask.mhd", "K:\\[transfer]\\GD301\\374\\GD301_woCA_3um_374N_density_map.mhd", size);
}