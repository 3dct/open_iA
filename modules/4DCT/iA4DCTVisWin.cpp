/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iA4DCTVisWin.h"

#include <iAMagicLens.h>
#include <iAVtkVersion.h>

#include "dlg_4DCTFileOpen.h"
#include "dlg_regionView.h"
#include "iA4DCTAllVisualizationsDockWidget.h"
#include "iA4DCTBoundingBoxDockWidget.h"
#include "iA4DCTData.h"
#include "iA4DCTDefectVisDockWidget.h"
#include "iA4DCTFileData.h"
#include "iA4DCTFractureVisDockWidget.h"
#include "iA4DCTMainWin.h"
#include "iA4DCTPlaneDockWidget.h"
#include "iA4DCTRegionViewDockWidget.h"
#include "iA4DCTSettings.h"
#include "iA4DCTToolsDockWidget.h"
#include "iABoundingBoxVisModule.h"
#include "iADefectVisModule.h"
#include "iAFractureVisModule.h"
#include "iAMhdFileInfo.h"
#include "iAPlaneVisModule.h"
#include "iARegionVisModule.h"
#include "iAVisModule.h"
#include "iAVisModuleItem.h"
#include "iA4DCTRegionMarkerModule.h"

#include <QFileDialog>
#include <QSettings>
#include <QString>
#include <QVector>

#include <itkBinaryThresholdImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkResampleImageFilter.h>

#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkLegendScaleActor.h>

iA4DCTVisWin::iA4DCTVisWin( iA4DCTMainWin * parent /*= 0*/ )
	: QMainWindow( parent )
	, m_currentStage(0)
	, m_mainWin( parent )
	, m_isVirgin( true )
{
	setupUi( this );
	// default size
	m_size[0] = 100; m_size[1] = 100; m_size[2] = 100;
	m_timer.setInterval( 1000 ); // 1 sec

	// GUI
	splitter->setStretchFactor( 0, 1 );
	splitter->setStretchFactor( 1, 0 );

	// setup renderer
	m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New( );
	qvtkWidget->SetMainRenderWindow( m_renderWindow );
	m_mainRen = vtkSmartPointer<vtkRenderer>::New( );
	m_mainRen->SetLayer( 0 );
	m_mainRen->SetBackground( 0.5, 0.5, 0.5 );
	m_mainRen->InteractiveOn( );
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	qvtkWidget->GetRenderWindow( )->AddRenderer( m_mainRen );
#else
	qvtkWidget->renderWindow()->AddRenderer(m_mainRen);
#endif
	m_magicLensRen = qvtkWidget->getLensRenderer( );

	m_renList = vtkSmartPointer<vtkRendererCollection>::New( );

	setOrientationWidgetEnabled( true );

	// setup dock widgets
	// tools
	m_dwTools = new iA4DCTToolsDockWidget( this );
	// settings
	m_dwBoundingBox = new iA4DCTBoundingBoxDockWidget( this );
	m_dwBoundingBox->setRenderer( m_mainRen );
	connect( m_dwBoundingBox, &iA4DCTBoundingBoxDockWidget::updateRenderWindow, this, &iA4DCTVisWin::updateRenderWindow);
	// fracture vis
	m_dwFractureVis = new iA4DCTFractureVisDockWidget( this );
	m_dwFractureVis->setData( m_mainWin->getStageData( ) );
	connect( m_dwFractureVis, &iA4DCTFractureVisDockWidget::updateRenderWindow, this, &iA4DCTVisWin::updateRenderWindow);
	// plane
	m_dwPlane = new iA4DCTPlaneDockWidget( this );
	connect( m_dwPlane, &iA4DCTPlaneDockWidget::updateRenderWindow, this, &iA4DCTVisWin::updateRenderWindow);
	// defect vis
	m_dwDefectVis = new iA4DCTDefectVisDockWidget( this );
	connect( m_dwDefectVis, &iA4DCTDefectVisDockWidget::updateRenderWindow, this, &iA4DCTVisWin::updateRenderWindow);
	// all visualizations view
	m_dwAllVis = new iA4DCTAllVisualizationsDockWidget( this );
	m_dwAllVis->setCollection( &m_visModules );
	connect( m_dwAllVis, &iA4DCTAllVisualizationsDockWidget::updateVisualizations, this, &iA4DCTVisWin::updateVisualizations);
	connect(m_dwAllVis, &iA4DCTAllVisualizationsDockWidget::selectedVisModule, this, &iA4DCTVisWin::selectedVisModule);
	// region visualization
	m_dwRegionVis = new iA4DCTRegionViewDockWidget( this );
	connect( m_dwRegionVis, &iA4DCTRegionViewDockWidget::updateRenderWindow, this, &iA4DCTVisWin::updateRenderWindow);

	// setup dock widget layout
	setTabPosition( Qt::LeftDockWidgetArea, QTabWidget::North );
	setTabPosition( Qt::RightDockWidgetArea, QTabWidget::North );
	addDockWidget( Qt::RightDockWidgetArea, m_dwAllVis );
	addDockWidget( Qt::RightDockWidgetArea, m_dwTools );
	addDockWidget( Qt::LeftDockWidgetArea, m_dwBoundingBox );
	addDockWidget( Qt::LeftDockWidgetArea, m_dwFractureVis );
	addDockWidget( Qt::LeftDockWidgetArea, m_dwPlane );
	addDockWidget( Qt::LeftDockWidgetArea, m_dwDefectVis );
	addDockWidget( Qt::LeftDockWidgetArea, m_dwRegionVis );

	setToolsDockWidgetsEnabled( false );

	// setup signals
	connect( m_dwTools->pbDefectViewerAdd, &QPushButton::clicked, this, &iA4DCTVisWin::addDefectView);
	connect( m_dwTools->pbDefectVisAdd, &QPushButton::clicked, this, &iA4DCTVisWin::addDefectVis);
	connect( m_dwTools->pbBoundingBoxAdd, &QPushButton::clicked, this, &iA4DCTVisWin::addBoundingBox);
	connect( m_dwTools->pbFractureViewerAdd, &QPushButton::clicked, this, &iA4DCTVisWin::onExtractButtonClicked);
	connect( m_dwTools->pbFractureViewerLoad, &QPushButton::clicked, this, &iA4DCTVisWin::onLoadButtonClicked);
	connect( m_dwTools->pbSurfaceViewerAdd, &QPushButton::clicked, this, &iA4DCTVisWin::addSurfaceVis);
	connect( m_dwTools->cbBackground, &iAColorBox::colorChanged, this, &iA4DCTVisWin::changeBackground);
	connect( actionMagicLens, &QAction::toggled, this, &iA4DCTVisWin::enableMagicLens);
	connect( sStage, &QSlider::valueChanged, this, &iA4DCTVisWin::onStageSliderValueChanged);
	connect( pbFirst, &QPushButton::clicked, this, &iA4DCTVisWin::onFirstButtonClicked);
	connect( pbPrevious, &QPushButton::clicked, this, &iA4DCTVisWin::onPreviousButtonClicked);
	connect( pbNext, &QPushButton::clicked, this, &iA4DCTVisWin::onNextButtonClicked);
	connect( pbLast, &QPushButton::clicked, this, &iA4DCTVisWin::onLastButtonClicked);
	connect( pbPlay, &QPushButton::clicked, this, &iA4DCTVisWin::onPlayButtonClicked);
	connect( &m_timer, &QTimer::timeout, this, &iA4DCTVisWin::onNextButtonClicked);
	connect( sbInterval, QOverload<int>::of(&QSpinBox::valueChanged), this, &iA4DCTVisWin::onIntervalValueChanged);
	connect( actionResetCam, &QAction::triggered, this, &iA4DCTVisWin::resetCamera);
	connect( actionXYView, &QAction::triggered, this, &iA4DCTVisWin::setXYView);
	connect( actionXZView, &QAction::triggered, this, &iA4DCTVisWin::setXZView);
	connect( actionYZView, &QAction::triggered, this, &iA4DCTVisWin::setYZView);
	connect( actionXYBackView, &QAction::triggered, this, &iA4DCTVisWin::setXYBackView);
	connect( actionXZBackView, &QAction::triggered, this, &iA4DCTVisWin::setXZBackView);
	connect( actionYZBackView, &QAction::triggered, this, &iA4DCTVisWin::setYZBackView);
	connect( actionOrientationMarker, &QAction::toggled, this, &iA4DCTVisWin::setOrientationWidgetEnabled);
	connect( actionOrientationMarker, &QAction::toggled, this, &iA4DCTVisWin::updateRenderWindow);
	connect( actionSideBySideView, &QAction::toggled, this, &iA4DCTVisWin::enableSideBySideView);

	iA4DCTRegionMarkerModule* regionMarker = new iA4DCTRegionMarkerModule;
	regionMarker->attachTo( m_mainRen );
	regionMarker->enable( );
	m_visModules.addModule( regionMarker, "Region marker" );
	m_dwAllVis->updateContext( );

	vtkSmartPointer<vtkLegendScaleActor> legendScaleActor = vtkSmartPointer<vtkLegendScaleActor>::New();
	m_mainRen->AddActor( legendScaleActor );
}

iA4DCTVisWin::~iA4DCTVisWin( )
{ }

void iA4DCTVisWin::setImageSize( double * size )
{
	m_size[0] = size[0]; m_size[1] = size[1]; m_size[2] = size[2];
	//const double dNear = 0.1;
	//double dFar = std::max( m_size[0], std::max( m_size[1], m_size[2] ) ) * 8;
}

void iA4DCTVisWin::setNumberOfStages( int number )
{
	sStage->setMaximum( number - 1 );

	for( int i = 0; i < number; i++ )
	{
		vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New( );
		ren->SetLayer( 0 );
		ren->SetBackground( 0.5, 0.5, 0.5 );
		ren->InteractiveOn( );
		m_renList->AddItem( ren );
	}
}

void iA4DCTVisWin::updateRenderWindow( )
{
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	qvtkWidget->GetRenderWindow( )->Render( );
#else
	qvtkWidget->renderWindow()->Render();
#endif
}

// show dialog to select an image file
bool iA4DCTVisWin::showDialog( iA4DCTFileData & fileData )
{
	/*int stageId;
	return showDialog( imagePath, stageId );*/

	dlg_4DCTFileOpen dialog( this );
	dialog.setData( m_mainWin->getStageData( ) );
	if( dialog.exec( ) != QDialog::Accepted )
		return false;
	fileData = dialog.getFile( );
	return true;
}

// load a surface from the input surface file
void iA4DCTVisWin::onLoadButtonClicked( )
{
	iA4DCTFileData fileData;
	if( !showDialog( fileData ) ) {
		return;
	}

	iAFractureVisModule* fractureView = new iAFractureVisModule;
	fractureView->attachTo( m_mainRen );
	fractureView->setSize( m_size );
	fractureView->load( fileData.Path );
	static int number = 0;
	m_visModules.addModule( fractureView, "Fracture (loaded) " + QString::number( number++ ) );
	m_dwAllVis->updateContext( );
}

// calculate a new surface from the input file
void iA4DCTVisWin::onExtractButtonClicked( )
{
	iA4DCTFileData fileData;
	if( !showDialog( fileData ) ) {
		return;
	}
	iAMhdFileInfo mhdFileInfo( fileData.Path );
	double imgSize[3], imgSpacing[3];
	mhdFileInfo.getFileDimSize( imgSize );
	mhdFileInfo.getElementSpacing( imgSpacing );
	double size[3] = { imgSize[0] * imgSpacing[0], imgSize[1] * imgSpacing[1], imgSize[2] * imgSpacing[2] };
	//extract(imagePath);

	iAFractureVisModule* fractureView = new iAFractureVisModule;
	if( actionAddToMagicLens->isChecked( ) )
	{
		fractureView->attachTo( m_magicLensRen );
	}
	else
	{
		fractureView->attachTo( m_mainRen );
	}
	fractureView->setSize( size );
	fractureView->extract( fileData.Path );
	static int number = 0;
	m_visModules.addModule( fractureView, "Fracture (extracted) " + QString::number( number++ ) );

	double bounds[6];
	fractureView->getBounds( bounds );
	iABoundingBoxVisModule * boundingBox = new iABoundingBoxVisModule;
	double bbSize[3] = { ( bounds[1] - bounds[0] ) / SCENE_SCALE, ( bounds[3] - bounds[2] ) / SCENE_SCALE, ( bounds[5] - bounds[4] ) / SCENE_SCALE };
	boundingBox->setSize( bbSize );

	if( actionAddToMagicLens->isChecked( ) )
	{
		boundingBox->attachTo( m_magicLensRen );
	}
	else
	{
		boundingBox->attachTo( m_mainRen );
	}
	boundingBox->setPosition( bounds[0], bounds[2], bounds[4] );
	m_visModules.addModule( boundingBox, "Fracture bounding box " + QString::number( number++ ) );

	m_dwAllVis->updateContext( );
}

void iA4DCTVisWin::addSurfaceVis( )
{
	dlg_regionView dialog( this );
	dialog.setData( m_mainWin->getStageData( ) );
	if( dialog.exec( ) != QDialog::Accepted )
		return;

	QString imagePath = dialog.getImagePath( );
	QString imageName = dialog.getImageName( );
	double threshold = dialog.getThreshold( );

	// add vis module
	iARegionVisModule * regionView = new iARegionVisModule;
	if( actionAddToMagicLens->isChecked( ) )
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
	m_visModules.addModule( regionView, "Final Fracture Surface (" + imageName + ") " + QString::number( number++ ) );
	m_dwAllVis->updateContext( );
}

void iA4DCTVisWin::onStageSliderValueChanged( int val )
{
	m_currentStage = val;
	m_dwAllVis->setCurrentStage( m_currentStage );
	m_dwAllVis->updateContext( );
	selectedVisModule( nullptr );

	updateVisualizations( );
}

void iA4DCTVisWin::addBoundingBox( )
{
	// add vis module
	iABoundingBoxVisModule * boundingBox = new iABoundingBoxVisModule;
	if( actionAddToMagicLens->isChecked( ) )
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
	m_dwAllVis->updateContext( );
}

void iA4DCTVisWin::addDefectView( )
{
	// open dialog
	iA4DCTFileData fileData;
	if( !showDialog( fileData ) ) {
		return;
	}

	// add vis module
	iAPlaneVisModule * planeVis = new iAPlaneVisModule;
	if( actionAddToMagicLens->isChecked( ) )
	{
		planeVis->attachTo( m_magicLensRen );
	}
	else
	{
		planeVis->attachTo( m_mainRen );
	}
	planeVis->setImage( fileData );
	planeVis->setSize( m_size );
	static int number = 0;
	m_visModules.addModule( planeVis, "Defect Viewer (" + fileData.Name + ") " + QString::number( number++ ) );
	m_dwAllVis->updateContext( );
}

void iA4DCTVisWin::addDefectVis( )
{
	// open dialog
	iA4DCTFileData fileData;
	if( !showDialog( fileData ) ) {
		return;
	}

	iADefectVisModule* defVis = new iADefectVisModule;
	defVis->setInputFile( fileData.Path );
	defVis->attachTo( m_mainRen );
	defVis->setColor( 1, 0.75, 0 );
	static int number = 0;
	m_visModules.addModule( defVis, "3D Defect Viewer (" + fileData.Name + ") " + QString::number( number++ ) );
	m_dwAllVis->updateContext( );
}

void iA4DCTVisWin::updateVisualizations( )
{
	QList<iAVisModuleItem *> modules = m_visModules.getModules( );
	for( auto m : modules ) {
		int index = m->stages.indexOf( m_currentStage );
		if( index != -1 ) {
			m->module->enable( );
			if( m_isVirgin ) { m_isVirgin = false; m_mainRen->ResetCamera( ); }
		} else {
			m->module->disable( );
		}
	}
	updateRenderWindow( );
}

void iA4DCTVisWin::selectedVisModule( iAVisModule * visModule )
{
	iARegionVisModule * regionVis = dynamic_cast<iARegionVisModule *>( visModule );
	iABoundingBoxVisModule * bbVis = dynamic_cast<iABoundingBoxVisModule *>( visModule );
	iAFractureVisModule * fractureVis = dynamic_cast<iAFractureVisModule *>( visModule );
	iAPlaneVisModule * planeVis = dynamic_cast<iAPlaneVisModule *>( visModule );
	iADefectVisModule * defectVis = dynamic_cast<iADefectVisModule *>( visModule );

	setToolsDockWidgetsEnabled( false );

	if( regionVis ) {
		m_dwRegionVis->setEnabled( true );
		m_dwRegionVis->attachTo( regionVis );
	} else if( bbVis ) {
		m_dwBoundingBox->setEnabled( true );
		m_dwBoundingBox->attachTo( bbVis );
	} else if( fractureVis ) {
		m_dwFractureVis->setEnabled( true );
		m_dwFractureVis->attachTo( fractureVis );
	} else if( planeVis ) {
		m_dwPlane->setEnabled( true );
		m_dwPlane->attachTo( planeVis );
	} else if( defectVis ) {
		m_dwDefectVis->setEnabled( true );
		m_dwDefectVis->attachTo( defectVis );
	}
}

void iA4DCTVisWin::onFirstButtonClicked( )
{
	sStage->setValue( sStage->minimum( ) );
}

void iA4DCTVisWin::onPreviousButtonClicked( )
{
	if( sStage->value( ) == sStage->minimum( ) ) {
		onLastButtonClicked( );
	} else {
		sStage->setValue( sStage->value( ) - 1 );
	}
}

void iA4DCTVisWin::onNextButtonClicked( )
{
	if( sStage->value( ) == sStage->maximum( ) ) {
		onFirstButtonClicked( );
	} else {
		sStage->setValue( sStage->value( ) + 1 );
	}
}

void iA4DCTVisWin::onLastButtonClicked( )
{
	sStage->setValue( sStage->maximum( ) );
}

void iA4DCTVisWin::onPlayButtonClicked( bool checked )
{
	if( checked ) {
		m_timer.start( );
	} else {
		m_timer.stop( );
	}
}

void iA4DCTVisWin::onIntervalValueChanged( int val )
{
	m_timer.setInterval( val );
}

void iA4DCTVisWin::setToolsDockWidgetsEnabled( bool enabled )
{
	m_dwFractureVis->setEnabled( enabled );
	m_dwPlane->setEnabled( enabled );
	m_dwRegionVis->setEnabled( enabled );
	m_dwBoundingBox->setEnabled( enabled );
	m_dwDefectVis->setEnabled( enabled );
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
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	qvtkWidget->GetRenderWindow( )->Render( );
#else
	qvtkWidget->renderWindow()->Render();
#endif
}

void iA4DCTVisWin::setXYView( )
{
	m_mainRen->GetActiveCamera( )->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera( )->SetPosition( 0, 0, 1 );
	m_mainRen->GetActiveCamera( )->SetViewUp( 0., -1., 0. );
	resetCamera( );
}

void iA4DCTVisWin::setXYBackView( )
{
	m_mainRen->GetActiveCamera( )->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera( )->SetPosition( 0, 0, -1 );
	m_mainRen->GetActiveCamera( )->SetViewUp( 0., -1., 0. );
	resetCamera( );
}

void iA4DCTVisWin::setXZView( )
{
	m_mainRen->GetActiveCamera( )->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera( )->SetPosition( 0, 1, 0 );
	m_mainRen->GetActiveCamera( )->SetViewUp( 0, 0., 1. );
	resetCamera( );
}

void iA4DCTVisWin::setXZBackView( )
{
	m_mainRen->GetActiveCamera( )->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera( )->SetPosition( 0, -1., 0. );
	m_mainRen->GetActiveCamera( )->SetViewUp( .0, 0., 1. );
	resetCamera( );
}

void iA4DCTVisWin::setYZView( )
{
	m_mainRen->GetActiveCamera( )->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera( )->SetPosition( 1, 0, 0 );
	m_mainRen->GetActiveCamera( )->SetViewUp( 0., 0., 1. );
	resetCamera( );
}

void iA4DCTVisWin::setYZBackView( )
{
	m_mainRen->GetActiveCamera( )->SetFocalPoint( 0, 0, 0 );
	m_mainRen->GetActiveCamera( )->SetPosition( -1, 0, 0 );
	m_mainRen->GetActiveCamera( )->SetViewUp( 0., 0., 1. );
	resetCamera( );
}

void iA4DCTVisWin::setOrientationWidgetEnabled( bool enabled )
{
	if( enabled )
	{
		if( m_orientWidget ) return;

		vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New( );
		m_orientWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New( );
		m_orientWidget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
		m_orientWidget->SetOrientationMarker( axes );
		m_orientWidget->SetInteractor( m_renderWindow->GetInteractor( ) );
		m_orientWidget->SetViewport( 0.0, 0.0, 0.2, 0.2 );
		m_orientWidget->SetEnabled( 1 );
		m_orientWidget->InteractiveOn( );
	}
	else
	{
		if( !m_orientWidget ) return;

		m_orientWidget->SetEnabled( false );
		m_orientWidget = nullptr;
	}
}

void iA4DCTVisWin::changeBackground( QColor col )
{
	m_mainRen->SetBackground( col.redF( ), col.greenF( ), col.blueF( ) );
	updateRenderWindow( );
}

void iA4DCTVisWin::enableSideBySideView( bool enabled )
{
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	qvtkWidget->GetRenderWindow( )->GetRenderers( )->RemoveAllItems( );
#else
	qvtkWidget->renderWindow()->GetRenderers()->RemoveAllItems();
#endif

	if( enabled )
	{
		double width = 1. / m_renList->GetNumberOfItems( );
		double start = 0.;

		vtkSmartPointer<vtkRenderer> screenRen = vtkSmartPointer<vtkRenderer>::New( );
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		qvtkWidget->GetRenderWindow( )->AddRenderer( screenRen );
#else
		qvtkWidget->renderWindow()->AddRenderer(screenRen);
#endif

		vtkSmartPointer<vtkCamera> cam = vtkSmartPointer<vtkCamera>::New( );
		cam->ShallowCopy( m_mainRen->GetActiveCamera( ) );

		m_renList->InitTraversal( );
		while( vtkRenderer* ren = m_renList->GetNextItem( ) )
		{
			ren->SetViewport( start, 0.5 - width / 2, start + width, 0.5 + width / 2 );
			//ren->SetViewport( start, 0, start + width, 1 );
			start += width;

			//ren->SetActiveCamera( m_mainRen->GetActiveCamera( ) );
			ren->SetActiveCamera( cam );
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
			qvtkWidget->GetRenderWindow( )->AddRenderer( ren );
#else
			qvtkWidget->renderWindow()->AddRenderer(ren);
#endif
		}

		QList<iAVisModuleItem *> modules = m_visModules.getModules( );
		for( auto m : modules ) {
			m->module->disable( );
			if( m->stages.count( ) == 0 ) continue;
			int step = m->stages[0];
			if( vtkRenderer* ren = static_cast<vtkRenderer*>( m_renList->GetItemAsObject( step ) ) ) {
				m->module->attachTo( ren );
				m->module->enable( );
			}
		}
		updateRenderWindow( );
	}
	else
	{
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		qvtkWidget->GetRenderWindow( )->AddRenderer( m_mainRen );
#else
		qvtkWidget->renderWindow()->AddRenderer(m_mainRen);
#endif
		QList<iAVisModuleItem *> modules = m_visModules.getModules( );
		for( auto m : modules ) {
			m->module->disable( );
			if( m->stages.count( ) == 0 ) continue;
			int step = m->stages[0];
			if( step == m_currentStage ) {
				m->module->attachTo( m_mainRen );
				m->module->enable( );
			}
		}
		updateRenderWindow( );
	}
}
