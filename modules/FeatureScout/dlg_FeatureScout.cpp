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
#include "dlg_blobVisualization.h"
#include "dlg_commoninput.h"
#include "dlg_editPCClass.h"
#include "dlg_FeatureScout.h"
#include "dlg_imageproperty.h"
#include "dlg_modalities.h"
#include "charts/iADiagramFctWidget.h"
#include "charts/iAQSplom.h"
#include "iAmat4.h"
#include "iABlobCluster.h"
#include "iABlobManager.h"
#include "iAMeanObjectTFView.h"
#include "iAModalityTransfer.h"
#include "iAMovieHelper.h"
#include "iAObjectAnalysisType.h"
#include "iAProgress.h"
#include "iARenderer.h"
#include "mdichild.h"

#include <itkAddImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkImageRegionIterator.h>
#include <itkImageToVTKImageFilter.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkLabelMapMaskImageFilter.h>
#include <itkPasteImageFilter.h>
#include <itkVTKImageToImageFilter.h>

#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAnnotationLink.h>
#include <vtkAxis.h>
#include <vtkImageCast.h>
#include <vtkCamera.h>
#include <vtkChart.h>
#include <vtkChartMatrix.h>
#include <vtkChartParallelCoordinates.h>
#include <vtkChartXY.h>
#include <vtkColorTransferFunction.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkCornerAnnotation.h>
#include <vtkCubeAxesActor.h>
#include <vtkDelaunay2D.h>
#include <vtkDynamic2DLabelMapper.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLookupTable.h>
#include <vtkMarchingCubes.h>
#include <vtkMath.h>
#include <vtkMathUtilities.h>
#include <vtkMetaImageWriter.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOutlineFilter.h>
#include <vtkPen.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlot.h>
#include <vtkPlotParallelCoordinates.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarsToColors.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkSphereSource.h>
#include <vtkStdString.h>
#include <vtkSTLWriter.h>
#include <vtkStringArray.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredGridGeometryFilter.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>
#include <vtkVariantArray.h>
#include <vtkVersion.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

#include "QtCore/qmath.h"
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QTableWidget>
#include <QTreeView>
#include <QProgressBar>

#include <cmath>

//Global defines for initial layout
const int initEExpPCPPHeight = 300;
const int initEExpWidth = 1000;
const int initPCWidth = 600;
const int initPPWidth = 330;

// global defines for using QXmlStream
const QString IFVTag( "IFV_Class_Tree" );
const QString ClassTag( "CLASS" );
const QString ObjectTag( "OBJECT" );
const QString VersionAttribute( "Version" );
const QString CountAllAttribute( "Number_Of_Objects" );
const QString NameAttribute( "NAME" );
const QString ColorAttribute( "COLOR" );
const QString CountAttribute( "COUNT" );
const QString PercentAttribute( "PERCENT" );
const QString LabelAttribute( "Label" );
const QString LabelAttributePore( "LabelId" );

#define VTK_CREATE(type, name) \
	vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void ColormapRGB( const double normal[3], double color_out[3] );
void ColormapCMY( const double normal[3], double color_out[3] );
void ColormapCMYNormalized( const double normal[3], double color_out[3] );
void ColormapRGBNormalized( const double normal[3], double color_out[3] );
void ColormapCMYAbsolute( const double normal[3], double color_out[3] );
void ColormapRGBAbsolute( const double normal[3], double color_out[3] );
void ColormapCMYAbsoluteNormalized( const double normal[3], double color_out[3] );
void ColormapRGBAbsoluteNormalized( const double normal[3], double color_out[3] );
void ColormapRGBHalfSphere( const double normal[3], double color_out[3] );

typedef void( *ColormapFuncPtr )( const double normal[3], double color_out[3] );
ColormapFuncPtr colormapsIndex[] =
{
	ColormapRGB,
	ColormapRGBNormalized,
	ColormapRGBAbsolute,
	ColormapRGBAbsoluteNormalized,

	ColormapCMY,
	ColormapCMYNormalized,
	ColormapCMYAbsolute,
	ColormapCMYAbsoluteNormalized,

	ColormapRGBHalfSphere,
};

dlg_FeatureScout::dlg_FeatureScout( MdiChild *parent, iAObjectAnalysisType fid, vtkRenderer* blobRen, vtkSmartPointer<vtkTable> csvtbl )
	: QDockWidget( parent ),
	oTF( parent->getPiecewiseFunction() ),
	cTF( parent->getColorTransferFunction() ),
	csvTable( csvtbl ),
	raycaster( parent->getRenderer() ),
	elementTableModel(nullptr),
	iovSPM(nullptr),
	iovPP(nullptr),
	iovPC(nullptr),
	iovDV(nullptr),
	iovMO(nullptr),
	matrix(nullptr),
	spmActivated(false),
	sourcePath( parent->currentFile() )
{
	setupUi( this );
	this->elementNr = csvTable->GetNumberOfColumns();
	this->objectNr = csvTable->GetNumberOfRows();
	this->activeChild = parent;
	this->filterID = fid;
	this->draw3DPolarPlot = false;
	this->enableRealTimeRendering = true;
	this->classRendering = true;
	this->setupPolarPlotResolution( 3.0 );

	this->raycaster = raycaster;
	blobManager = new iABlobManager();
	blobManager->SetRenderers( blobRen, this->raycaster->GetLabelRenderer() );
	double bounds[6];
	raycaster->GetImageDataBounds( bounds );
	blobManager->SetBounds( bounds );
	blobManager->SetProtrusion( 1.5 );
	int dimens[3] = { 50, 50, 50 };
	blobManager->SetDimensions( dimens );

	lut = vtkSmartPointer<vtkLookupTable>::New();
	chartTable = vtkSmartPointer<vtkTable>::New();
	chartTable->DeepCopy( csvTable );
	this->updateColumnNames();
	tableList.clear();
	tableList.push_back( chartTable );
	colorList.clear();
	selectedObjID.clear();
	columnVisArr = vtkSmartPointer<vtkStringArray>::New();
	columnVisArr->SetNumberOfValues( elementNr );

	initParallelCoordinates( fid );
	// connect IFV module to mdiChild dockwidgets
	this->pcWidget = iovPC->dockWidgetContents;
	this->pcPolarPlot = iovPP->plotWidget;
	this->orientationColorMapSelection = iovPP->colorMapSelection;
	this->orientColormap = iovPP->comboBox;

	// Initialize the models for QtViews
	this->setupViews();
	this->setupModel();
	this->setupConnections();
	blobVisDialog = new dlg_blobVisualization();

	// set first column of the classTreeView to minimal (not stretched)
	this->classTreeView->resizeColumnToContents( 0 );
	this->classTreeView->header()->setStretchLastSection( false );
	this->elementTableView->resizeColumnsToContents();
	this->classTreeView->setExpandsOnDoubleClick( false );
}

dlg_FeatureScout::~dlg_FeatureScout()
{
	delete blobManager;

	if ( this->elementTableModel != 0 )
	{
		delete elementTableModel;
		elementTableModel = 0;
	}

	if ( this->classTreeModel != 0 )
	{
		delete classTreeModel;
		classTreeModel = 0;
	}

	if ( this->activeChild != 0 )
		activeChild = 0;

	if ( this->oTF != 0 )
		oTF = 0;

	if ( this->cTF != 0 )
		cTF = 0;

	if ( this->PolarPlotPolyData != 0 )
		PolarPlotPolyData = 0;

	if ( this->delaunay != 0 )
		delaunay = 0;

	this->deletePcViewPointer();

	if ( this->spmActivated )
		delete matrix;
}

void dlg_FeatureScout::pcViewMouseButtonCallBack( vtkObject * obj, unsigned long,
														 void * client_data, void *, vtkCommand * command )
{
	// Gets the mouse button event for pcChart and holds the SPM-Annotations consistent with PC-Annoatations.
	if ( this->spmActivated )
	{
		// TODO SPM MS
		// matrix->UpdateCustomLegend();
		// matrix->GetAnnotationLink()->SetCurrentSelection( pcChart->GetAnnotationLink()->GetCurrentSelection() );
		
		
		//matrix->setSelection() 
		////pcChart->GetAnnotationLink()
		//vtkSmartPointer<vtkAnnotationLink> myAnnotation = this->pcChart->GetAnnotationLink(); 
		//int text = (int) this->pcChart->GetAnnotationLink()->GetCurrentSelection()->GetNumberOfNodes(); 
		//this->pcChart->setA
		//end experimenting MS TODO REMOVE comments

		//annotationLink is null;
		vtkSmartPointer<vtkIdTypeArray> DataSelection = this->pcChart->GetPlot(0)->GetSelection(); 
		
		vtkIdType val = DataSelection->GetDataTypeValueMax(); 

		//QVector<uint> *
		
		//QSharedPointer<QVector<uint>> selID( new QVector<uint>); 
		
		QVector<uint> selID

		//matrix->setSelection()
		//64bit data type
		/*vtkIdType maxID = DataSelection->GetMaxId();
		vtkIdType minID = DataSelection->GetDataTypeValueMin()*/; 
		int countSelection = DataSelection->GetNumberOfValues(); ;
		int idx = 0;

		vtkVariant var_Idx = 0; 
		uint objID = 0; 
		/*
			// add new class
			for ( int i = 0; i < CountObject; i++ )
			{
				// get objID from item->text()
				vtkVariant v = pcChart->GetPlot( 0 )->GetSelection()->GetVariantValue( i );
				objID = v.ToInt() + 1;	//fibre index starting at 1 n
		
		
		*/
		if (countSelection > 0) {

			for (idx; idx < countSelection; idx++) {

				var_Idx = DataSelection->GetVariantValue(idx);
				//fiber starts with index 1!!, mininum is 0
				//todo change 
				objID =  (unsigned int)var_Idx.ToLongLong() +1;
				
				
				//selID->push_back(objID); 
				selID.push_back(objID);

			}

			//matrix->selectionModified(&(*selID));
			
			matrix->setSelection(&selID);

			
			//update view
		}
		
		this->RealTimeRendering( this->pcChart->GetPlot( 0 )->GetSelection(), this->enableRealTimeRendering );
	}
	else
		this->RealTimeRendering( this->pcChart->GetPlot( 0 )->GetSelection(), this->enableRealTimeRendering );
}

void dlg_FeatureScout::setupNewPcView( bool lookupTable )
{
	this->pcWidget->setEnabled( true );

	// copy csvTable to chartTable, columnNames should be changed if we have fibre visualization
	// lookupTable set to true for multi rendering, draw a color ParallelCoordinates
	if ( lookupTable )
	{
		chartTable->DeepCopy( csvTable );
		this->updateColumnNames();
	}

	this->pcWidget->SetRenderWindow( NULL );
	this->deletePcViewPointer();

	// new initialize of the pcChart
	this->pcView = vtkContextView::New();
	this->pcChart = vtkChartParallelCoordinates::New();

	if ( this->spmActivated )
		// TODO SPM
		//this->pcChart->SetAnnotationLink( matrix->GetAnnotationLink() ); //annLink

	// setup interactor and render window
	this->pcView->SetInteractor( this->pcWidget->GetInteractor() );
	this->pcWidget->SetRenderWindow( this->pcView->GetRenderWindow() );
	this->pcChart->GetPlot( 0 )->SetInputData( chartTable );

	//pcChart->GetAxis(0)->GetLabelProperties()->SetFontSize(22);

	// Line smoothness
	this->pcChart->GetPlot( 0 )->GetPen()->SetOpacity( 90 );

	this->pcChart->GetPlot( 0 )->SetWidth( 0.1 );
	//this->pcView->GetRenderWindow()->SetMultiSamples(0);

	this->pcView->GetScene()->AddItem( pcChart );

	//Creates a popup menu
	QMenu* popup2 = new QMenu( pcWidget );
	popup2->addAction( "Add Class" );
	popup2->setStyleSheet( "font-size: 11px; background-color: #9B9B9B; border: 1px solid black;" );

	connect( popup2, SIGNAL( triggered( QAction* ) ), this, SLOT( spPopupSelection( QAction* ) ) );

	vtkEventQtSlotConnect *pcConnections = vtkEventQtSlotConnect::New();

	// Gets right button release event (on a parallel coordinates).
	pcConnections->Connect( pcWidget->GetRenderWindow()->GetInteractor(),
							vtkCommand::RightButtonReleaseEvent,
							this,
							SLOT( spPopup( vtkObject*, unsigned long, void*, void*, vtkCommand* ) ),
							popup2, 1.0 );

	// Gets right button press event (on a scatter plot).
	pcConnections->Connect( pcWidget->GetRenderWindow()->GetInteractor(),
							vtkCommand::RightButtonPressEvent,
							this,
							SLOT( spBigChartMouseButtonPressed( vtkObject*, unsigned long, void*, void*, vtkCommand* ) ) );

	// Sets up a connection for an selection changed event.
	pcConnections->Connect( pcChart,
							vtkCommand::SelectionChangedEvent,
							this,
							SLOT( pcViewMouseButtonCallBack( vtkObject*, unsigned long, void*, void*, vtkCommand* ) ) );

	this->updatePCColumnVisibility();

	if ( lookupTable )
	{
		this->pcWidget->setEnabled( false );
		//QMessageBox msgBox;
		//msgBox.setText("ParallelCoordinates view is disabled for multi cluster rendering. Please click any class to continue the classification.");
		//msgBox.setWindowTitle("ParallelCoordinates View");
		//msgBox.exec();
	}
}

void dlg_FeatureScout::updatePCColumnValues( QStandardItem *item )
{
	// new solution for using individual itemchagend signal
	if ( item->isCheckable() )
	{
		int i = item->index().row();

		if ( item->checkState() )
		{
			// use eleString.count() > 0 to specify that we have a fibre classificaiton
			// cause the column names in chartTable are manually changed
			if ( eleString.count() > 0 )
				columnVisArr->SetValue( i, eleString.at( i ).toStdString() );
			else
				columnVisArr->SetValue( i, elementTable->GetValue( i, 0 ).ToString() );
		}
		else
			columnVisArr->SetValue( i, "open_iA" );

		this->updatePCColumnVisibility();

		if ( this->spmActivated )
			this->spUpdateSPColumnVisibility();
	}
}

void dlg_FeatureScout::updatePCColumnVisibility()
{
	pcChart->SetColumnVisibilityAll( false );

	for ( int j = 0; j < elementNr; j++ )
	{
		const vtkStdString str = columnVisArr->GetValue( j );

		if ( str != "open_iA" )
		{
			if ( eleString.count() > 0 )
				pcChart->SetColumnVisibility( eleString.at( j ).toStdString(), true );
			else
				pcChart->SetColumnVisibility( elementTable->GetValue( j, 0 ).ToString(), true );
		}
	}
	this->pcView->ResetCamera();
	this->pcView->Render();
}

void dlg_FeatureScout::setupDefaultElement()
{
	if ( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION )                      //Fibers
	{
		pcChart->SetColumnVisibilityAll( false );
		pcChart->SetColumnVisibility( eleString.at( 7 ).toStdString(), true );	//a11
		pcChart->SetColumnVisibility( eleString.at( 8 ).toStdString(), true );	//a22
		pcChart->SetColumnVisibility( eleString.at( 9 ).toStdString(), true );	//a33
		pcChart->SetColumnVisibility( eleString.at( 13 ).toStdString(), true );	//theta
		pcChart->SetColumnVisibility( eleString.at( 14 ).toStdString(), true );	//phi
		pcChart->SetColumnVisibility( eleString.at( 15 ).toStdString(), true );	//xm
		pcChart->SetColumnVisibility( eleString.at( 16 ).toStdString(), true );	//ym
		pcChart->SetColumnVisibility( eleString.at( 17 ).toStdString(), true );	//zm
		pcChart->SetColumnVisibility( eleString.at( 18 ).toStdString(), true );	//straightlength
		//pcChart->SetColumnVisibility(eleString.at(20).toStdString(), true);	//diameter
		//pcChart->SetColumnVisibility(eleString.at(22).toStdString(), true);	//volume
	}
	else                                                                         //Pores
	{
		pcChart->SetColumnVisibilityAll( false );
		pcChart->SetColumnVisibility( eleString.at( 0 ).toStdString(), true );	//id
		pcChart->SetColumnVisibility( eleString.at( 13 ).toStdString(), true );	//dimx
		pcChart->SetColumnVisibility( eleString.at( 14 ).toStdString(), true );	//dimy
		pcChart->SetColumnVisibility( eleString.at( 15 ).toStdString(), true );	//dimz
		pcChart->SetColumnVisibility( eleString.at( 16 ).toStdString(), true );	//phi
		pcChart->SetColumnVisibility( eleString.at( 17 ).toStdString(), true );	//theata
		pcChart->SetColumnVisibility( eleString.at( 18 ).toStdString(), true );	//Xm
		pcChart->SetColumnVisibility( eleString.at( 19 ).toStdString(), true );	//Ym
		pcChart->SetColumnVisibility( eleString.at( 20 ).toStdString(), true );	//Zm
		pcChart->SetColumnVisibility( eleString.at( 21 ).toStdString(), true );	//volume
		pcChart->SetColumnVisibility( eleString.at( 22 ).toStdString(), true );	//roundness
		pcChart->SetColumnVisibility( eleString.at( 28 ).toStdString(), true );	//MajorLength
	}
}

void dlg_FeatureScout::setupModel()
{
	// setup header data
	elementTableModel->setHeaderData( 0, Qt::Horizontal, tr( "Element" ) );
	elementTableModel->setHeaderData( 1, Qt::Horizontal, tr( "Minimal" ) );
	elementTableModel->setHeaderData( 2, Qt::Horizontal, tr( "Maximal" ) );
	elementTableModel->setHeaderData( 3, Qt::Horizontal, tr( "Average" ) );

	// initialize checkboxes for the first column
	for ( int i = 0; i < elementNr; i++ )
	{
		//Fibers - a11, a22, a33, theta, phi, xm, ym, zm, straightlength, diameter, volume
		if ( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION && ( i == 7 || i == 8 ||
			i == 9 || i == 13 || i == 14 || i == 15 || i == 16 || i == 17 || i == 18 ) )
		{
			elementTableModel->setData( elementTableModel->index( i, 0, QModelIndex() ), Qt::Checked, Qt::CheckStateRole );
			columnVisArr->SetValue( i, chartTable->GetColumnName( i ) );
		}
		//Pores - volume, dimx, dimy, dimz, posx, posy, posz, shapefactor
		else if ( this->filterID == INDIVIDUAL_PORE_VISUALIZATION &&
				( i == 0 || i == 13 || i == 14 || i == 15 || i == 16 || i == 17 || i == 18 || i == 19 || i == 20 ||
				  i == 21 || i == 28 ) )
		{
			elementTableModel->setData( elementTableModel->index( i, 0, QModelIndex() ), Qt::Checked, Qt::CheckStateRole );
			columnVisArr->SetValue( i, chartTable->GetColumnName( i ) );
		}
		else
		{
			elementTableModel->setData( elementTableModel->index( i, 0, QModelIndex() ), Qt::Unchecked, Qt::CheckStateRole );
			columnVisArr->SetValue( i, "open_iA" );
		}
		elementTableModel->item( i, 0 )->setCheckable( true );
	}

	// initialize element values
	this->calculateElementTable();
	this->initElementTableModel();

	// class tree model
	// set up class tree headeritems
	this->activeClassItem = new QStandardItem();
	classTreeModel->setHorizontalHeaderItem( 0, new QStandardItem( "Class" ) );
	classTreeModel->setHorizontalHeaderItem( 1, new QStandardItem( "Count" ) );
	classTreeModel->setHorizontalHeaderItem( 2, new QStandardItem( "Percent" ) );

	// initialize children for default class
	this->initClassTreeModel();
}

void dlg_FeatureScout::setupViews()
{
	// declare element table model
	elementTableModel = new QStandardItemModel( elementNr, 4, this );
	elementTable = vtkSmartPointer<vtkTable>::New();

	// declare class tree model
	classTreeModel = new QStandardItemModel();

	//init Distribution View
	m_dvContextView = vtkSmartPointer<vtkContextView>::New();

	// initialize the QtTableView
	this->classTreeView = new QTreeView();
	this->elementTableView = new QTableView();

	// setup context menu
	this->classTreeView->setContextMenuPolicy( Qt::CustomContextMenu );

	// set Widgets for the table views
	this->ClassLayout->addWidget( this->classTreeView );
	this->ElementLayout->addWidget( this->elementTableView );
	// set models
	this->elementTableView->setModel( elementTableModel );
	this->classTreeView->setModel( classTreeModel );
	//	this->elementTableView->setSortingEnabled(true);

	// preparing chart and view for Parallel Coordinates
	this->pcView = vtkContextView::New();
	this->pcChart = vtkChartParallelCoordinates::New();

	// setup interactor and render window
	this->pcView->SetInteractor( this->pcWidget->GetInteractor() );
	this->pcWidget->SetRenderWindow( this->pcView->GetRenderWindow() );

	this->pcChart->GetPlot( 0 )->SetInputData( chartTable );

	// Line smoothness
	this->pcChart->GetPlot( 0 )->GetPen()->SetOpacity( 90 );
	this->pcChart->GetPlot( 0 )->SetWidth( 0.1 );
	//this->pcView->GetRenderWindow()->SetMultiSamples(0);

	this->pcView->GetScene()->AddItem( pcChart );
	this->setupDefaultElement();

	// Creates a popup menu
	QMenu* popup2 = new QMenu( pcWidget );
	popup2->addAction( "Add Class" );
	popup2->setStyleSheet( "font-size: 11px; background-color: #9B9B9B; border: 1px solid black;" );

	connect( popup2, SIGNAL( triggered( QAction* ) ), this, SLOT( spPopupSelection( QAction* ) ) );

	vtkEventQtSlotConnect *pcConnections = vtkEventQtSlotConnect::New();

	// Gets right button release event (on a parallel coordinates).
	pcConnections->Connect( pcWidget->GetRenderWindow()->GetInteractor(),
							vtkCommand::RightButtonReleaseEvent,
							this,
							SLOT( spPopup( vtkObject*, unsigned long, void*, void*, vtkCommand* ) ),
							popup2, 1.0 );

	// Gets right button press event (on a scatter plot).
	pcConnections->Connect( pcWidget->GetRenderWindow()->GetInteractor(),
							vtkCommand::RightButtonPressEvent,
							this,
							SLOT( spBigChartMouseButtonPressed( vtkObject*, unsigned long, void*, void*, vtkCommand* ) ) );

	//this->createSphereView();
	this->setupPolarPlotView( chartTable );
}

void dlg_FeatureScout::calculateElementTable()
{
	// free table contents first
	elementTable->Initialize();

	double range[2] = { 0, 1 };
	VTK_CREATE( vtkStringArray, nameArr );  // name of columns, both for set visuability of columns in PC view and for element table
	VTK_CREATE( vtkFloatArray, v1Arr );	// minimum
	VTK_CREATE( vtkFloatArray, v2Arr );	// maximal
	VTK_CREATE( vtkFloatArray, v3Arr );	// average
	nameArr->SetNumberOfValues( elementNr );
	v1Arr->SetNumberOfValues( elementNr );
	v2Arr->SetNumberOfValues( elementNr );
	v3Arr->SetNumberOfValues( elementNr );

	// convert IDs in String to Int
	nameArr->SetValue( 0, csvTable->GetColumnName( 0 ) );
	v1Arr->SetValue( 0, chartTable->GetColumn( 0 )->GetVariantValue( 0 ).ToFloat() );
	v2Arr->SetValue( 0, chartTable->GetColumn( 0 )->GetVariantValue( chartTable->GetNumberOfRows() - 1 ).ToFloat() );
	v3Arr->SetValue( 0, 0 );

	for ( int i = 1; i < elementNr; i++ )
	{
		nameArr->SetValue( i, csvTable->GetColumnName( i ) );
		vtkDataArray *mmr = vtkDataArray::SafeDownCast( chartTable->GetColumn( i ) );
		mmr->GetRange( range );
		v1Arr->SetValue( i, range[0] );
		v2Arr->SetValue( i, range[1] );
		v3Arr->SetValue( i, this->calculateAverage( mmr ) );
	}

	// add new values
	elementTable->AddColumn( nameArr );
	elementTable->AddColumn( v1Arr );
	elementTable->AddColumn( v2Arr );
	elementTable->AddColumn( v3Arr );
}

void dlg_FeatureScout::initElementTableModel( int idx )
{
	// here try to convert the vtkTable values into a QtStandardItemModel
	// this should be done every time when initialize realtime values for elementTable
	if ( idx < 0 )
	{
		elementTableModel->setHeaderData( 1, Qt::Horizontal, tr( "Minimal" ) );
		elementTableView->showColumn( 2 );
		elementTableView->showColumn( 3 );

		for ( int i = 0; i < elementNr; i++ ) // number of rows
		{
			for ( int j = 0; j < 4; j++ )
			{
				vtkVariant v = elementTable->GetValue( i, j );
				QString str;
				if ( j == 0 )
				{
					str = QString::fromUtf8( v.ToUnicodeString().utf8_str() ).trimmed();
				}
				else
				{
					if ( i == 0 || i == elementNr - 1 )
						str = QString::number( v.ToInt() );
					else
						str = QString::number( v.ToDouble(), 'f', 2 );
				}
				elementTableModel->setData( elementTableModel->index( i, j, QModelIndex() ), str );
				// Surpresses changeability of items.
				elementTableModel->item( i, j )->setEditable( false );
			}
		}
	}
	else
	{
		// if the values of objectID is given and bigger than zero
		// we then know that is a single selection, and we want to update the
		// element Table view with the new values
		elementTableModel->setHeaderData( 1, Qt::Horizontal, tr( "Value" ) );
		elementTableView->hideColumn( 2 );
		elementTableView->hideColumn( 3 );

		for ( int i = 0; i < elementNr; i++ )
		{
			vtkVariant v = chartTable->GetValue( idx, i );
			QString str = QString::number( v.ToDouble(), 'f', 2 );

			if ( i == 0 || i == elementNr - 1 )
				str = QString::number( v.ToInt() );

			//QString str = QString::fromUtf8(v.ToUnicodeString().utf8_str()).trimmed();
			elementTableModel->setData( elementTableModel->index( i, 1, QModelIndex() ), str );
		}
	}
}

void dlg_FeatureScout::initClassTreeModel()
{
	QStandardItem *rootItem = classTreeModel->invisibleRootItem();
	QList<QStandardItem *> stammItem = prepareRow( "Unclassified", QString( "%1" ).arg( objectNr ), "100" );
	stammItem.first()->setData( QColor( "darkGray" ), Qt::DecorationRole );
	this->colorList.append( QColor( "darkGray" ) );

	rootItem->appendRow( stammItem );
	for ( int i = 0; i < objectNr; ++i )
	{
		vtkVariant v = chartTable->GetColumn( 0 )->GetVariantValue( i );
		QStandardItem *item = new QStandardItem( QString::fromUtf8( v.ToUnicodeString().utf8_str() ).trimmed() );
		stammItem.first()->appendRow( item );
	}
	this->activeClassItem = stammItem.first();
}

// calculate the average value of a 1D array
float dlg_FeatureScout::calculateAverage( vtkDataArray *arr )
{
	double av = 0.0;
	double sum = 0.0;

	for ( int i = 0; i < arr->GetNumberOfTuples(); i++ )
		sum = sum + arr->GetVariantValue( i ).ToDouble();

	av = sum / arr->GetNumberOfTuples();
	return av;
}

QList<QStandardItem *> dlg_FeatureScout::prepareRow( const QString &first, const QString &second, const QString &third )
{
	// prepare the class header rows for class tree view first grade child unter rootitem
	// for adding child object to this class, use item.first()->appendRow()
	QList<QStandardItem *> rowItems;
	rowItems << new QStandardItem( first );
	rowItems << new QStandardItem( second );
	rowItems << new QStandardItem( third );
	return rowItems;
}

// setup connections, define signal and slots
void dlg_FeatureScout::setupConnections()
{
	// create ClassTreeView context menu actions
	blobRendering = new QAction( tr( "Enable blob rendering" ), this->classTreeView );
	blobRemoveRendering = new QAction( tr( "Disable blob rendering" ), this->classTreeView );
	saveBlobMovie = new QAction( tr( "Save blob movie" ), this->classTreeView );
	objectAdd = new QAction( tr( "Add object" ), this->classTreeView );
	objectDelete = new QAction( tr( "Delete object" ), this->classTreeView );

	connect( this->blobRendering, SIGNAL( triggered() ), this, SLOT( EnableBlobRendering() ) );
	connect( this->blobRemoveRendering, SIGNAL( triggered() ), this, SLOT( DisableBlobRendering() ) );
	connect( this->saveBlobMovie, SIGNAL( triggered() ), this, SLOT( SaveBlobMovie() ) );
	connect( this->objectAdd, SIGNAL( triggered() ), this, SLOT( addObject() ) );
	connect( this->objectDelete, SIGNAL( triggered() ), this, SLOT( deleteObject() ) );
	connect( this->classTreeView, SIGNAL( customContextMenuRequested( const QPoint & ) ), this, SLOT( showContextMenu( const QPoint & ) ) );
	connect( this->add_class, SIGNAL( clicked() ), this, SLOT( ClassAddButton() ) );
	connect( this->save_class, SIGNAL( released() ), this, SLOT( ClassSaveButton() ) );
	connect( this->load_class, SIGNAL( released() ), this, SLOT( ClassLoadButton() ) );
	connect( this->delete_class, SIGNAL( clicked() ), this, SLOT( ClassDeleteButton() ) );
	connect( this->wisetex_save, SIGNAL( released() ), this, SLOT( WisetexSaveButton() ) );
	connect( this->csv_dv, SIGNAL( released() ), this, SLOT( CsvDVSaveButton() ) );
	/*connect(this->rendering, SIGNAL(clicked()), this, SLOT(RenderingButton()));
	connect(this->blob_rendering, SIGNAL(clicked()), this, SLOT(OpenBlobVisDialog()));
	connect(this->plot_scatter, SIGNAL(clicked()), this, SLOT(ScatterPlotButton()));*/


	// Set up other options
	//connect(this->option_Box, SIGNAL(currentIndexChanged(int)), this, SLOT(pcChangeOptions(int )));
	// set up realtime rendering for parallel coordinates brushing rendering
	//connect(this->pcWidget, SIGNAL(this->pcWidget), this, SLOT(RealTimeRendering()));

	// Element checkstate for visualizing Columns in PC view
	// Define ranges for PC columns
	connect( this->elementTableModel, SIGNAL( itemChanged( QStandardItem * ) ), this, SLOT( updatePCColumnValues( QStandardItem * ) ) );
	connect( this->classTreeView, SIGNAL( clicked( QModelIndex ) ), this, SLOT( classClicked( QModelIndex ) ) );
	connect( this->classTreeView, SIGNAL( activated( QModelIndex ) ), this, SLOT( classClicked( QModelIndex ) ) );
	connect( this->classTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( classDoubleClicked( QModelIndex ) ) );

	// Creates signal from qVtkWidget on a selection changed event
	// and sends it to the callback.
	vtkEventQtSlotConnect *pcConnections = vtkEventQtSlotConnect::New();
	pcConnections->Connect( pcChart,
							vtkCommand::SelectionChangedEvent,
							this,
							SLOT( pcViewMouseButtonCallBack( vtkObject*, unsigned long, void*, void*, vtkCommand* ) ), 0, 0.0, Qt::UniqueConnection );
}

void dlg_FeatureScout::pcChangeOptions( int idx )
{
	//this->raycaster->GetRenderer()->RemoveActor(scalarBar);

	switch ( idx )
	{
		case 0:			// option menu, do nothing
			break;

		case 1:			// blob Rendering, trigger Slot OpenBlobVisDialog()
			this->OpenBlobVisDialog();
			//this->option_Box->setCurrentIndex(0);
			break;

		case 3:			// multi Rendering
			this->RenderingButton();
			//this->option_Box->setCurrentIndex(0);
			this->classRendering = false;
			break;

		case 4:			// probability Rendering
			this->RenderingMeanObject();
			this->classRendering = false;
			break;

		case 5:			// orientation Rendering
			this->RenderingOrientation();
			this->classRendering = false;
			break;

		case 6:			// plot scatterplot matrix
			this->spmActivated = true;
			this->ScatterPlotButton();
			//this->option_Box->setCurrentIndex(0);
			break;

		case 7:
			this->RenderingFLD();
			//this->option_Box->setCurrentIndex(0);
			this->classRendering = false;
			break;

		default:
			break;
	}
}

void dlg_FeatureScout::RenderingButton()
{
	//Turns off FLD scalar bar updates polar plot view
	if ( m_scalarWidgetFLD != NULL )
	{
		m_scalarWidgetFLD->Off();
		this->updatePolarPlotColorScalar( chartTable );
	}

	QStandardItem *rootItem = this->classTreeModel->invisibleRootItem();
	int classCount = rootItem->rowCount();

	if ( classCount == 1 )
		return;

	double backAlpha = 0.00005;
	double backRGB[3];
	backRGB[0] = colorList.at( 0 ).redF();
	backRGB[1] = colorList.at( 0 ).greenF();
	backRGB[2] = colorList.at( 0 ).blueF();

	//double alpha = 1.0;
	double alpha = this->calculateOpacity( rootItem );
	double red = 0.0;
	double green = 0.0;
	double blue = 0.0;
	int CID = 0;

	// clear existing points
	this->oTF->RemoveAllPoints();
	this->cTF->RemoveAllPoints();

	// set background opacity and color
	this->oTF->ClampingOff();
	this->cTF->ClampingOff();

	// Iterate trough all classes to render, starting with 0 unclassified, 1 Class1,...
	for ( int i = 0; i < classCount; i++ )
	{
		// alpha = colorList.at(i).alpha()/255.0;
		red = colorList.at( i ).redF();
		green = colorList.at( i ).greenF();
		blue = colorList.at( i ).blueF();

		QStandardItem *item = rootItem->child( i, 0 );
		int itemL = item->rowCount();

		// Class has no objects, proceed with next class
		if ( !itemL )
			continue;

		int hid = 0, next_hid = 1;
		bool starting = false;

		for ( int j = 0; j < itemL; ++j )
		{
			hid = item->child( j, 0 )->text().toInt();

			if ( ( j + 1 ) < itemL )
			{
				next_hid = item->child( j + 1, 0 )->text().toInt();
			}
			else
			{
				if ( starting )
				{
					oTF->AddPoint( hid, alpha, 0.5, 1.0 );
					oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					break;
				}
				else
				{
					oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
					oTF->AddPoint( hid, alpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					break;
				}
			}

			//Create one single tooth
			if ( next_hid > hid + 1 && !starting )
			{
				oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
				cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			}
			else if ( next_hid == hid + 1 && !starting )
			{
				starting = true;
				oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
			}
			else if ( next_hid == hid + 1 && starting )
				continue;

			else if ( next_hid > hid + 1 && starting )
			{
				starting = false;
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			}
		}

		if ( hid < objectNr )
		{
			this->oTF->AddPoint( objectNr + 0.3, backAlpha, 0.5, 1.0 );
			this->cTF->AddRGBPoint( objectNr + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
		}
	}

	// update lookup table in PC View
	this->updateLookupTable( alpha );
	this->setupNewPcView( true );
	static_cast<vtkPlotParallelCoordinates *>( pcChart->GetPlot( 0 ) )->SetScalarVisibility( 1 );
	static_cast<vtkPlotParallelCoordinates *>( pcChart->GetPlot( 0 ) )->SetLookupTable( lut );
	static_cast<vtkPlotParallelCoordinates *>( pcChart->GetPlot( 0 ) )->SelectColorArray( "Class_ID" );
	this->pcChart->SetSize( pcChart->GetSize() );

	//Updates SPM
	if ( this->spmActivated )
	{
		// TODO SPM
		// matrix->GetAnnotationLink()->GetCurrentSelection()->RemoveAllNodes();
		// matrix->SetClass2Plot( -1 );
		// matrix->UpdateLayout();
	}

	static_cast<MdiChild*>( activeChild )->updateViews();

	//Cause of performance turrned off
	//static_cast<MdiChild*>(activeChild)->redrawHistogram();
}

void dlg_FeatureScout::SingleRendering( int idx )
{
	int cID = this->activeClassItem->index().row();
	int itemL = this->activeClassItem->rowCount();
	double red = colorList.at( cID ).redF(),
		   green = colorList.at( cID ).greenF(),
		   blue = colorList.at( cID ).blueF(),
		   alpha = 0.5,
		   backAlpha = 0.0,
		   backRGB[3] = { 0.0, 0.0, 0.0 };

	// clear existing points
	this->oTF->RemoveAllPoints();
	this->cTF->RemoveAllPoints();

	// set background opacity and color with clamping off
	this->oTF->ClampingOff();
	this->cTF->ClampingOff();
	this->oTF->AddPoint( 0, backAlpha );
	this->cTF->AddRGBPoint( 0, backRGB[0], backRGB[1], backRGB[2] );

	if ( idx > 0 ) // for single object selection
	{
		if ( ( idx - 1 ) >= 0 )
		{
			this->oTF->AddPoint( idx - 0.5, backAlpha );
			this->oTF->AddPoint( idx - 0.49, alpha );
			this->cTF->AddRGBPoint( idx - 0.5, backRGB[0], backRGB[1], backRGB[2] );
			this->cTF->AddRGBPoint( idx - 0.49, red, green, blue );
		}
		oTF->AddPoint( idx, alpha );
		cTF->AddRGBPoint( idx, red, green, blue );
		if ( ( idx + 1 ) <= objectNr )
		{
			this->oTF->AddPoint( idx + 0.3, backAlpha );
			this->oTF->AddPoint( idx + 0.29, alpha );
			this->cTF->AddRGBPoint( idx + 0.3, backRGB[0], backRGB[1], backRGB[2] );
			this->cTF->AddRGBPoint( idx + 0.29, red, green, blue );
		}
	}
	else // for single class selection
	{
		int hid = 0, next_hid = 1;
		bool starting = false;

		for ( int j = 0; j < itemL; ++j )
		{
			hid = this->activeClassItem->child( j, 0 )->text().toInt();

			if ( j + 1 < itemL )
				next_hid = this->activeClassItem->child( j + 1, 0 )->text().toInt();
			else
			{
				if ( starting )
				{
					oTF->AddPoint( hid, alpha, 0.5, 1.0 );
					oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					break;
				}
				else
				{
					oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
					oTF->AddPoint( hid, alpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					break;
				}
			}

			//Create one single tooth
			if ( next_hid > hid + 1 && !starting )
			{
				oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
				cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			}
			else if ( next_hid == hid + 1 && !starting )
			{
				starting = true;
				oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
			}
			else if ( next_hid == hid + 1 && starting )
				continue;
			else if ( next_hid > hid + 1 && starting )
			{
				starting = false;
				oTF->AddPoint( hid, alpha, 0.5, 1.0 );
				oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
				cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			}
		}

		if ( hid < objectNr )
		{
			this->oTF->AddPoint( objectNr + 0.3, backAlpha, 0.5, 1.0 );
			this->cTF->AddRGBPoint( objectNr + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
		}
	}
	raycaster->update();
}

void dlg_FeatureScout::RealTimeRendering( vtkIdTypeArray *selection, bool enabled )
{
	if ( enabled )
	{
		//Turns off FLD scalar bar updates polar plot view
		if ( m_scalarWidgetFLD != NULL )
		{
			m_scalarWidgetFLD->Off();
			this->updatePolarPlotColorScalar( chartTable );
		}

		this->orientationColorMapSelection->hide();
		this->orientColormap->hide();

		int countClass = this->activeClassItem->rowCount();
		int countSelection = selection->GetNumberOfTuples();

		if ( countClass > 0 )
		{
			double red = 0.0, green = 0.0, blue = 0.0, alpha = 0.5, backAlpha = 0.00, backRGB[3], classRGB[3], selRGB[3];
			backRGB[0] = 0.5; backRGB[1] = 0.5; backRGB[2] = 0.5;
			selRGB[0] = 1.0; selRGB[1] = 0.0; selRGB[2] = 0.0;	//selection color
			classRGB[0] = colorList.at( activeClassItem->index().row() ).redF();
			classRGB[1] = colorList.at( activeClassItem->index().row() ).greenF();
			classRGB[2] = colorList.at( activeClassItem->index().row() ).blueF();

			// clear existing points
			this->oTF->RemoveAllPoints();
			this->cTF->RemoveAllPoints();

			this->oTF->ClampingOff();
			this->cTF->ClampingOff();

			this->oTF->AddPoint( 0, backAlpha, 0.5, 1.0 );
			this->cTF->AddRGBPoint( 0, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );

			int hid = 0, next_hid = 1, selectionIndex = 0, previous_selectionIndex = 0;
			bool starting = false, hid_isASelection = false, previous_hid_isASelection = false;

			for ( int j = 0; j < countClass; ++j )
			{
				hid = this->activeClassItem->child( j )->text().toInt();

				if ( countSelection > 0 )
				{
					if ( j == selection->GetVariantValue( selectionIndex ).ToInt() )
					{
						hid_isASelection = true;
						red = selRGB[0], green = selRGB[1], blue = selRGB[2];

						if ( selectionIndex + 1 < selection->GetNumberOfTuples() )
							selectionIndex++;
					}
					else
					{
						hid_isASelection = false;
						red = classRGB[0]; green = classRGB[1]; blue = classRGB[2];
					}

					if ( j > 0 )
					{
						if ( j - 1 == selection->GetVariantValue( previous_selectionIndex ).ToInt() )
						{
							previous_hid_isASelection = true;

							if ( previous_selectionIndex + 1 < selection->GetNumberOfTuples() )
								previous_selectionIndex++;
						}
						else
							previous_hid_isASelection = false;
					}
				}
				else
				{
					red = classRGB[0]; green = classRGB[1]; blue = classRGB[2];
				}

				// If we are not yet at the last object (of the class) get the next hid
				if ( ( j + 1 ) < countClass )
				{
					next_hid = this->activeClassItem->child( j + 1 )->text().toInt();
				}
				else	// If hid = the last object (of the class) we have to set the last object points
				{
					if ( starting )	// If we are in a sequence we have to set the ending (\)
					{
						oTF->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
						oTF->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
						oTF->AddPoint( hid, alpha, 0.5, 1.0 );
						oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );

						if ( hid_isASelection )
						{
							cTF->AddRGBPoint( hid - 0.5, 1.0, 0.0, 0.0, 0.5, 1.0 );
							cTF->AddRGBPoint( hid, 1.0, 0.0, 0.0, 0.5, 1.0 );
							cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
						}
						else
						{
							cTF->AddRGBPoint( hid - 0.5, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
							cTF->AddRGBPoint( hid, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
							cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
						}

						if ( previous_hid_isASelection )
							cTF->AddRGBPoint( hid - 1 + 0.3, 1.0, 0.0, 0.0, 0.5, 1.0 );
						else
							cTF->AddRGBPoint( hid - 1 + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
						break;
					}
					else	// if we are not in a sequence we have to create the last tooth (/\)
					{
						oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
						oTF->AddPoint( hid, alpha, 0.5, 1.0 );
						oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );

						cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
						cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
						cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
						break;
					}
				}

				if ( next_hid > hid + 1 && !starting )		//Create one single tooth
				{
					oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
					oTF->AddPoint( hid, alpha, 0.5, 1.0 );
					oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				}
				else if ( next_hid == hid + 1 && !starting )	//Creates the beginning of a sequence (/)
				{
					starting = true;
					oTF->AddPoint( hid - 0.5, backAlpha, 0.5, 1.0 );
					oTF->AddPoint( hid, alpha, 0.5, 1.0 );
					cTF->AddRGBPoint( hid - 0.5, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
					cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
				}
				else if ( next_hid == hid + 1 && starting )	//Continous the started sequence (-)
				{
					if ( !hid_isASelection && previous_hid_isASelection )
					{
						cTF->AddRGBPoint( hid - 1 + 0.3, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
						cTF->AddRGBPoint( hid - 0.5, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );
						cTF->AddRGBPoint( hid + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );

						oTF->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
						oTF->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
						oTF->AddPoint( hid + 0.3, alpha, 0.5, 1.0 );
					}
					else if ( hid_isASelection && !previous_hid_isASelection )
					{
						cTF->AddRGBPoint( hid - 0.5, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
						cTF->AddRGBPoint( hid + 0.3, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
						cTF->AddRGBPoint( hid - 1 + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );

						oTF->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
						oTF->AddPoint( hid + 0.3, alpha, 0.5, 1.0 );
						oTF->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
					}
				}
				else if ( next_hid > hid + 1 && starting )	//  (\)
				{
					starting = false;

					oTF->AddPoint( hid - 1 + 0.3, alpha, 0.5, 1.0 );
					oTF->AddPoint( hid - 0.5, alpha, 0.5, 1.0 );
					oTF->AddPoint( hid, alpha, 0.5, 1.0 );
					oTF->AddPoint( hid + 0.3, backAlpha, 0.5, 1.0 );

					if ( previous_hid_isASelection )
						cTF->AddRGBPoint( hid - 1 + 0.3, selRGB[0], selRGB[1], selRGB[2], 0.5, 1.0 );
					else
						cTF->AddRGBPoint( hid - 1 + 0.3, classRGB[0], classRGB[1], classRGB[2], 0.5, 1.0 );

					cTF->AddRGBPoint( hid - 0.5, red, green, blue, 0.5, 1.0 );
					cTF->AddRGBPoint( hid, red, green, blue, 0.5, 1.0 );
					cTF->AddRGBPoint( hid + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
				}
			}

			if ( hid < objectNr )	// Creates the very last points (for all objects)  if it's not created yet
			{
				this->oTF->AddPoint( objectNr + 0.3, backAlpha, 0.5, 1.0 );
				this->cTF->AddRGBPoint( objectNr + 0.3, backRGB[0], backRGB[1], backRGB[2], 0.5, 1.0 );
			}
			MdiChild * mdiChild = static_cast<MdiChild*>(activeChild);
			mdiChild->updateViews();
			//raycaster->update();
		}
	}
}

void dlg_FeatureScout::RenderingMeanObject()
{
	int classCount = classTreeModel->invisibleRootItem()->rowCount();
	if ( classCount < 2 )	// unclassified class only
	{
		QMessageBox msgBox;
		msgBox.setText( "No defined class (except the 'unclassified' class)." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	MdiChild * mdiChild = static_cast<MdiChild*>( activeChild );
	mdiChild->initProgressBar();

	// Delete old mean object data lists
	for ( int i = 0; i < m_MOData.moHistogramList.size(); i++ )
		delete m_MOData.moHistogramList[i];
	m_MOData.moVolumesList.clear();
	m_MOData.moHistogramList.clear();
	m_MOData.moRendererList.clear();
	m_MOData.moVolumeMapperList.clear();
	m_MOData.moVolumePropertyList.clear();
	m_MOData.moImageDataList.clear();

	// Casts image to long (if necessary) and convert it to an ITK image
	typedef itk::Image< long, DIM > IType;
	typedef itk::VTKImageToImageFilter<IType> VTKToITKConnector;
	VTKToITKConnector::Pointer vtkToItkConverter = VTKToITKConnector::New();
	if ( static_cast<MdiChild*>( activeChild )->getImagePointer()->GetScalarType() != 8 )	// long type
	{
		vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
		cast->SetInputData( static_cast<MdiChild*>( activeChild )->getImagePointer() );
		cast->SetOutputScalarTypeToLong();
		cast->Update();
		vtkToItkConverter->SetInput( cast->GetOutput() );
	}
	else
		vtkToItkConverter->SetInput( static_cast<MdiChild*>( activeChild )->getImagePointer() );
	vtkToItkConverter->Update();

	IType::Pointer itkImageData = vtkToItkConverter->GetOutput();
	double spacing[3];
	spacing[0] = static_cast<MdiChild*>( activeChild )->getImagePointer()->GetSpacing()[0];
	spacing[1] = static_cast<MdiChild*>( activeChild )->getImagePointer()->GetSpacing()[1];
	spacing[2] = static_cast<MdiChild*>( activeChild )->getImagePointer()->GetSpacing()[2];
	itk::Size<DIM> itkImageDataSize = itkImageData->GetLargestPossibleRegion().GetSize();

	typedef itk::BinaryThresholdImageFilter <IType, IType> BinaryThresholdImageFilterType;
	BinaryThresholdImageFilterType::Pointer thresholdFilter = BinaryThresholdImageFilterType::New();
	thresholdFilter->SetInput( itkImageData );
	thresholdFilter->SetLowerThreshold( 0 );
	thresholdFilter->SetUpperThreshold( 0 );
	thresholdFilter->SetInsideValue( 0 );
	thresholdFilter->SetOutsideValue( 1 );
	thresholdFilter->Update();
	typedef itk::LabelObject< long, DIM > LabelObjectType;
	typedef itk::LabelMap< LabelObjectType > LabelMapType;
	typedef itk::LabelImageToLabelMapFilter< IType, LabelMapType> I2LType;
	I2LType::Pointer i2l = I2LType::New();
	i2l->SetInput( itkImageData );

	typedef itk::LabelMapMaskImageFilter< LabelMapType, IType > MaskType;
	MaskType::Pointer mask = MaskType::New();
	mask->SetInput( i2l->GetOutput() );
	mask->SetFeatureImage( thresholdFilter->GetOutput() );
	mask->SetBackgroundValue( 0 );
	mask->SetCrop( true );

	// Defines mean object output image
	typedef long MObjectImagePixelType;
	typedef itk::Image< MObjectImagePixelType, DIM > MObjectImageType;
	MObjectImageType::RegionType outputRegion;
	MObjectImageType::RegionType::IndexType outputStart;
	outputStart[0] = 0; outputStart[1] = 0; outputStart[2] = 0;
	itk::Size<DIM> moImgSize;
	itk::Size<DIM> moImgCenter;
	for ( int i = 0; i < DIM; ++i )
	{
		itkImageDataSize[i] % 2 == 0 ?
			moImgSize[i] = itkImageDataSize[i] + 1 :
			moImgSize[i] = itkImageDataSize[i];
		moImgCenter[i] = std::round( moImgSize[i] / 2.0 );
	}
	outputRegion.SetSize( moImgSize );
	outputRegion.SetIndex( outputStart );
	MObjectImageType::Pointer mObjectITKImage = MObjectImageType::New();
	mObjectITKImage->SetRegions( outputRegion );
	const MObjectImageType::SpacingType& out_spacing = itkImageData->GetSpacing();
	const MObjectImageType::PointType& inputOrigin = itkImageData->GetOrigin();
	double outputOrigin[DIM];
	for ( unsigned int i = 0; i < DIM; i++ )
		outputOrigin[i] = inputOrigin[i];
	mObjectITKImage->SetSpacing( out_spacing );
	mObjectITKImage->SetOrigin( outputOrigin );
	mObjectITKImage->Allocate();

	// Defines add image
	typedef long addImagePixelType;
	typedef itk::Image< addImagePixelType, DIM > addImageType;
	addImageType::RegionType addoutputRegion;
	addImageType::RegionType::IndexType addoutputStart;
	addoutputStart[0] = 0; addoutputStart[1] = 0; addoutputStart[2] = 0;
	addoutputRegion.SetSize( moImgSize );
	addoutputRegion.SetIndex( outputStart );
	addImageType::Pointer addImage = addImageType::New();
	addImage->SetRegions( addoutputRegion );
	const addImageType::SpacingType& addout_spacing = itkImageData->GetSpacing();
	const addImageType::PointType& addinputOrigin = itkImageData->GetOrigin();
	double addoutputOrigin[DIM];
	for ( unsigned int i = 0; i < DIM; i++ )
		addoutputOrigin[i] = addinputOrigin[i];
	addImage->SetSpacing( addout_spacing );
	addImage->SetOrigin( addoutputOrigin );
	addImage->Allocate();

	for ( int currClass = 1; currClass < classCount; ++currClass )
	{
		std::map<int, int>* meanObjectIds = new std::map<int, int>();
		for ( int j = 0; j < classTreeModel->invisibleRootItem()->child( currClass )->rowCount(); ++j )
		{
			if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
				meanObjectIds->operator[]( tableList[currClass]->GetValue( j, 0 ).ToInt() ) =
				tableList[currClass]->GetValue( j, 0 ).ToFloat();
			else
				meanObjectIds->operator[]( tableList[currClass]->GetValue( j, 0 ).ToInt() ) =
				tableList[currClass]->GetValue( j, 0 ).ToFloat();
		}

		typedef itk::ImageRegionIterator< MObjectImageType> IteratorType;
		IteratorType mOITKImgIt( mObjectITKImage, outputRegion );
		for ( mOITKImgIt.GoToBegin(); !mOITKImgIt.IsAtEnd(); ++mOITKImgIt )
			mOITKImgIt.Set( 0 );

		typedef itk::ImageRegionIterator< addImageType> IteratorType;
		IteratorType addImgIt( addImage, addoutputRegion );
		for ( addImgIt.GoToBegin(); !addImgIt.IsAtEnd(); ++addImgIt )
			addImgIt.Set( 0 );

		int progress = 0;
		std::map<int, int>::const_iterator it;
		for ( it = meanObjectIds->begin(); it != meanObjectIds->end(); ++it )
		{
			mask->SetLabel( it->first );
			mask->Update();
			itk::Size<DIM> maskSize;
			maskSize = mask->GetOutput()->GetLargestPossibleRegion().GetSize();

			IType::IndexType destinationIndex;
			destinationIndex[0] = moImgCenter[0] - std::round( maskSize[0] / 2 );
			destinationIndex[1] = moImgCenter[1] - std::round( maskSize[1] / 2 );
			destinationIndex[2] = moImgCenter[2] - std::round( maskSize[2] / 2 );
			typedef itk::PasteImageFilter <IType, MObjectImageType > PasteImageFilterType;
			PasteImageFilterType::Pointer pasteFilter = PasteImageFilterType::New();
			pasteFilter->SetSourceImage( mask->GetOutput() );
			pasteFilter->SetDestinationImage( mObjectITKImage );
			pasteFilter->SetSourceRegion( mask->GetOutput()->GetLargestPossibleRegion() );
			pasteFilter->SetDestinationIndex( destinationIndex );

			typedef itk::AddImageFilter <MObjectImageType, MObjectImageType > AddImageFilterType;
			AddImageFilterType::Pointer addFilter = AddImageFilterType::New();
			addFilter->SetInput1( addImage );
			addFilter->SetInput2( pasteFilter->GetOutput() );
			addFilter->Update();
			addImage = addFilter->GetOutput();

			double percentage = round( ( currClass - 1 ) * 100.0 / ( classCount - 1 ) +
									   ( progress + 1.0 ) * ( 100.0 / ( classCount - 1 ) ) / meanObjectIds->size() );
			mdiChild->updateProgressBar( percentage );
			QCoreApplication::processEvents();
			progress++;
		}

		// Normalize voxels values to 1
		typedef itk::Image< float, DIM > moOutputImageType;
		typedef itk::CastImageFilter< addImageType, moOutputImageType > CastFilterType;
		CastFilterType::Pointer caster = CastFilterType::New();
		caster->SetInput( addImage );
		caster->Update();
		typedef itk::ImageRegionIterator< moOutputImageType> casterIteratorType;
		casterIteratorType casterImgIt( caster->GetOutput(), caster->GetOutput()->GetLargestPossibleRegion() );
		for ( casterImgIt.GoToBegin(); !casterImgIt.IsAtEnd(); ++casterImgIt )
			casterImgIt.Set( casterImgIt.Get() / meanObjectIds->size() );

		// Convert resulting MObject ITK image to an VTK image
		typedef itk::ImageToVTKImageFilter<moOutputImageType> ITKTOVTKConverterType;
		ITKTOVTKConverterType::Pointer itkToVTKConverter = ITKTOVTKConverterType::New();
		itkToVTKConverter->SetInput( caster->GetOutput() );
		itkToVTKConverter->Update();
		vtkSmartPointer<vtkImageData> meanObjectImage = vtkSmartPointer<vtkImageData>::New();
		meanObjectImage->DeepCopy( itkToVTKConverter->GetOutput() );
		m_MOData.moImageDataList.append( meanObjectImage );

		// Create histogram and TFs for each MObject
		QString moHistName = classTreeModel->invisibleRootItem()->child( currClass, 0 )->text();
		if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
			moHistName.append( " Fiber Mean Object" );
		else
			moHistName.append( " Void Mean Object" );
		iAModalityTransfer* moHistogram = new iAModalityTransfer( m_MOData.moImageDataList[currClass - 1]->GetScalarRange() );
		m_MOData.moHistogramList.append( moHistogram );

		// Create MObject default Transfer Tunctions
		if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION ) // Fibers
		{
			m_MOData.moHistogramList[currClass-1]->GetColorFunction()->AddRGBPoint( 0.0, 0.0, 0.0, 0.0 );
			m_MOData.moHistogramList[currClass-1]->GetColorFunction()->AddRGBPoint( 0.01, 1.0, 1.0, 0.0 );
			m_MOData.moHistogramList[currClass-1]->GetColorFunction()->AddRGBPoint( 0.095, 1.0, 1.0, 0.0 );
			m_MOData.moHistogramList[currClass-1]->GetColorFunction()->AddRGBPoint( 0.1, 0.0, 0.0, 1.0 );
			m_MOData.moHistogramList[currClass-1]->GetColorFunction()->AddRGBPoint( 1.00, 0.0, 0.0, 1.0 );
			m_MOData.moHistogramList[currClass-1]->GetOpacityFunction()->AddPoint( 0.0, 0.0 );
			m_MOData.moHistogramList[currClass-1]->GetOpacityFunction()->AddPoint( 0.01, 0.01 );
			m_MOData.moHistogramList[currClass-1]->GetOpacityFunction()->AddPoint( 0.095, 0.01 );
			m_MOData.moHistogramList[currClass-1]->GetOpacityFunction()->AddPoint( 0.1, 0.05 );
			m_MOData.moHistogramList[currClass-1]->GetOpacityFunction()->AddPoint( 1.00, 0.18 );
		}
		else // Voids
		{
			m_MOData.moHistogramList[currClass - 1]->GetColorFunction()->AddRGBPoint( 0.0, 0.0, 0.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->GetColorFunction()->AddRGBPoint( 0.0001, 0.0, 0.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->GetColorFunction()->AddRGBPoint( 0.001, 1.0, 1.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->GetColorFunction()->AddRGBPoint( 0.18, 1.0, 1.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->GetColorFunction()->AddRGBPoint( 0.2, 0.0, 0.0, 1.0 );
			m_MOData.moHistogramList[currClass - 1]->GetColorFunction()->AddRGBPoint( 1.0, 0.0, 0.0, 1.0 );
			m_MOData.moHistogramList[currClass - 1]->GetOpacityFunction()->AddPoint( 0.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->GetOpacityFunction()->AddPoint( 0.0001, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->GetOpacityFunction()->AddPoint( 0.001, 0.005 );
			m_MOData.moHistogramList[currClass - 1]->GetOpacityFunction()->AddPoint( 0.18, 0.005 );
			m_MOData.moHistogramList[currClass - 1]->GetOpacityFunction()->AddPoint( 0.2, 0.08 );
			m_MOData.moHistogramList[currClass - 1]->GetOpacityFunction()->AddPoint( 1.0, 0.5 );
		}

		// Create the property and attach the transfer functions
		vtkSmartPointer<vtkVolumeProperty> vProperty = vtkSmartPointer<vtkVolumeProperty>::New();
		m_MOData.moVolumePropertyList.append( vProperty );
		vProperty->SetColor( m_MOData.moHistogramList[currClass - 1]->GetColorFunction() );
		vProperty->SetScalarOpacity( m_MOData.moHistogramList[currClass - 1]->GetOpacityFunction() );
		vProperty->SetInterpolationTypeToLinear();
		vProperty->ShadeOff();

		// Create volume and mapper and set input for mapper
		vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
		m_MOData.moVolumesList.append( volume );
		vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> mapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
		m_MOData.moVolumeMapperList.append( mapper );
		mapper->SetAutoAdjustSampleDistances( 1 );
		mapper->SetSampleDistance( 1.0 );
		mapper->SetInputData( m_MOData.moImageDataList[currClass - 1] );
		mapper->Update();
		mapper->UpdateDataObject();
		volume->SetProperty( m_MOData.moVolumePropertyList[currClass-1] );
		volume->SetMapper( m_MOData.moVolumeMapperList[currClass-1] );
		volume->Update();
	}

	// Create the outline for volume
	vtkSmartPointer<vtkOutlineFilter> outline = vtkSmartPointer<vtkOutlineFilter>::New();
	outline->SetInputData( m_MOData.moVolumesList[0]->GetMapper()->GetDataObjectInput() );
	vtkSmartPointer<vtkPolyDataMapper> outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	outlineMapper->SetInputConnection( outline->GetOutputPort() );
	vtkSmartPointer<vtkActor> outlineActor = vtkSmartPointer<vtkActor>::New();
	outlineActor->SetMapper( outlineMapper );
	outlineActor->GetProperty()->SetColor( 0, 0, 0 );
	outlineActor->GetProperty()->SetLineWidth( 1.0 );
	outlineActor->GetProperty()->SetOpacity( 0.1 );

	// Calculates the max dimension of the image
	double maxDim = 0.0;
	for ( int i = 0; i < 6; ++i )
	{
		if ( outlineActor->GetBounds()[i] > maxDim )
			maxDim = outlineActor->GetBounds()[i];
	}

	// Setup Mean Object view
	if ( !iovMO )
	{
		iovMO = new dlg_IOVMO( this );
		connect( iovMO->pb_ModTF, SIGNAL( clicked() ), this, SLOT( modifyMeanObjectTF() ) );
		connect( iovMO->tb_OpenDataFolder, SIGNAL( clicked() ), this, SLOT( browseFolderDialog() ) );
		connect( iovMO->tb_SaveStl, SIGNAL( clicked() ), this, SLOT( saveStl() ) );

		// Create a render window and an interactor for all the MObjects
		m_renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
		iovMO->plotWidget->SetRenderWindow( m_renderWindow );
		vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
		renderWindowInteractor->SetRenderWindow( m_renderWindow );
		vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
		renderWindowInteractor->SetInteractorStyle( style );

		if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
			iovMO->setWindowTitle( QString( "Fiber Mean Object View" ) );
		else
			iovMO->setWindowTitle( QString( "Void Mean Object View" ) );

		mdiChild->addDockWidget( Qt::RightDockWidgetArea, iovMO );
		iovMO->show();
	}

	// Update MOClass comboBox
	iovMO->cb_Classes->clear();
	for ( int i = 1; i < classCount; ++i )
		iovMO->cb_Classes->addItem( classTreeModel->invisibleRootItem()->child( i, 0 )->text() );

	if ( iovSPM )
	{
		mdiChild->tabifyDockWidget(iovSPM, iovMO );
		iovMO->show();
		iovMO->raise();
	}
	else if ( iovDV )
	{
		mdiChild->tabifyDockWidget( iovDV, iovMO );
		iovMO->show();
		iovMO->raise();
	}

	// Remove old renderers
	iovMO->plotWidget->GetRenderWindow()->GetRenderers()->RemoveAllItems();

	// Define viewport variables
	unsigned numberOfMeanObjectVolumes = m_MOData.moVolumesList.size();
	float viewportColumns = numberOfMeanObjectVolumes < 3.0 ? fmod( numberOfMeanObjectVolumes, 3.0 ) : 3.0;
	float viewportRows = ceil( numberOfMeanObjectVolumes / viewportColumns );
	float fieldLengthX = 1.0 / viewportColumns, fieldLengthY = 1.0 / viewportRows;

	// Set up viewports
	for ( unsigned i = 0; i < viewportColumns * viewportRows; ++i )
	{
		vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
		m_MOData.moRendererList.append( renderer );
		renderer->GetActiveCamera()->ParallelProjectionOn();
		renderer->SetBackground( 1.0, 1.0, 1.0 );
		m_renderWindow->AddRenderer( m_MOData.moRendererList[i] );
		renderer->SetViewport( fmod( i, viewportColumns ) * fieldLengthX,
							   1 - ( ceil( ( i + 1.0 ) / viewportColumns ) / viewportRows ),
							   fmod( i, viewportColumns ) * fieldLengthX + fieldLengthX,
							   1 - ( ceil( ( i + 1.0 ) / viewportColumns ) / viewportRows ) + fieldLengthY );

		if ( i < m_MOData.moVolumesList.size() )
		{
			renderer->AddVolume( m_MOData.moVolumesList[i] );
			renderer->SetActiveCamera( raycaster->GetRenderer()->GetActiveCamera() );
			renderer->GetActiveCamera()->SetParallelScale( maxDim );	//use maxDim for right scaling to fit the data in the viewports

			vtkSmartPointer<vtkCornerAnnotation> cornerAnnotation = vtkSmartPointer<vtkCornerAnnotation>::New();
			cornerAnnotation->SetLinearFontScaleFactor( 2 );
			cornerAnnotation->SetNonlinearFontScaleFactor( 1 );
			cornerAnnotation->SetMaximumFontSize( 25 );
			cornerAnnotation->SetText( 2, classTreeModel->invisibleRootItem()->child( i + 1, 0 )->text().toStdString().c_str() );
			cornerAnnotation->GetTextProperty()->SetColor( colorList.at( i + 1 ).redF(), colorList.at( i + 1 ).greenF(), colorList.at( i + 1 ).blueF() );
			cornerAnnotation->GetTextProperty()->BoldOn();

			vtkSmartPointer<vtkCubeAxesActor> cubeAxesActor = vtkSmartPointer<vtkCubeAxesActor>::New();
			cubeAxesActor->SetBounds( outlineActor->GetBounds() );
			cubeAxesActor->SetCamera( renderer->GetActiveCamera() );
			cubeAxesActor->SetFlyModeToOuterEdges();
			cubeAxesActor->SetTickLocationToOutside();
			cubeAxesActor->SetScreenSize( 10.0 );	//changes axes font size
#if (VTK_MAJOR_VERSION > 7 || (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION > 0))
			cubeAxesActor->SetGridLineLocation( vtkCubeAxesActor::VTK_GRID_LINES_FURTHEST );
#else
			cubeAxesActor->SetGridLineLocation( VTK_GRID_LINES_FURTHEST );
#endif
			cubeAxesActor->DrawXGridlinesOn();  cubeAxesActor->DrawYGridlinesOn(); 	cubeAxesActor->DrawZGridlinesOn();
			cubeAxesActor->GetTitleTextProperty( 0 )->SetColor( 1.0, 0.0, 0.0 );
			cubeAxesActor->GetLabelTextProperty( 0 )->SetColor( 1.0, 0.0, 0.0 );
			cubeAxesActor->GetXAxesGridlinesProperty()->SetColor( 0.3, 0.3, 0.3 );
			cubeAxesActor->SetXUnits( "microns" );
			cubeAxesActor->GetTitleTextProperty( 1 )->SetColor( 0.0, 1.0, 0.0 );
			cubeAxesActor->GetLabelTextProperty( 1 )->SetColor( 0.0, 1.0, 0.0 );
			cubeAxesActor->GetYAxesGridlinesProperty()->SetColor( 0.3, 0.3, 0.3 );
			cubeAxesActor->SetYUnits( "microns" );
			cubeAxesActor->GetTitleTextProperty( 2 )->SetColor( 0.0, 0.0, 1.0 );
			cubeAxesActor->GetLabelTextProperty( 2 )->SetColor( 0.0, 0.0, 1.0 );
			cubeAxesActor->GetZAxesGridlinesProperty()->SetColor( 0.3, 0.3, 0.3 );
			cubeAxesActor->SetZUnits( "microns" );
			cubeAxesActor->XAxisLabelVisibilityOn(); cubeAxesActor->XAxisTickVisibilityOn(); cubeAxesActor->XAxisMinorTickVisibilityOff();
			cubeAxesActor->GetXAxesLinesProperty()->SetColor( 1.0, 0.0, 0.0 );
			cubeAxesActor->YAxisLabelVisibilityOn(); cubeAxesActor->YAxisTickVisibilityOn(); cubeAxesActor->YAxisMinorTickVisibilityOff();
			cubeAxesActor->GetYAxesLinesProperty()->SetColor( 0.0, 1.0, 0.0 );
			cubeAxesActor->ZAxisLabelVisibilityOn(); cubeAxesActor->ZAxisTickVisibilityOn(); cubeAxesActor->ZAxisMinorTickVisibilityOff();
			cubeAxesActor->GetZAxesLinesProperty()->SetColor( 0.0, 0.0, 1.0 );

			renderer->AddViewProp( cornerAnnotation );
			renderer->AddActor( cubeAxesActor );
			renderer->AddActor( outlineActor );
		}
		m_renderWindow->Render();
	}
}

void dlg_FeatureScout::modifyMeanObjectTF()
{
	m_motfView = new iAMeanObjectTFView( this );
	if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
		m_motfView->setWindowTitle( QString( iovMO->cb_Classes->itemText( iovMO->cb_Classes->currentIndex() ) + " Fiber Mean Object Transfer Function" ));
	else
		m_motfView->setWindowTitle( QString( iovMO->cb_Classes->itemText( iovMO->cb_Classes->currentIndex() ) + " Void Mean Object Transfer Function" ) );
	iADiagramFctWidget* histogram = static_cast<MdiChild*>(activeChild)->getHistogram();
	connect( histogram, SIGNAL( updateViews() ), this, SLOT( updateMOView() ) );
	m_motfView->horizontalLayout->addWidget( histogram );
	histogram->show();
	m_motfView->show();
}

void dlg_FeatureScout::updateMOView()
{
	iovMO->plotWidget->GetRenderWindow()->Render();
}

void dlg_FeatureScout::browseFolderDialog()
{
	QString dir = sourcePath;
	dir.truncate( sourcePath.lastIndexOf( "/" ) );
	QString filename = QFileDialog::getSaveFileName( this, tr( "Save STL File" ), dir, tr( "CSV Files (*.stl *.STL)" ) );
	if ( filename.isEmpty() )
		return;
	iovMO->le_StlPath->setText( filename );
}

void dlg_FeatureScout::saveStl()
{
	if ( iovMO->le_StlPath->text().isEmpty() )
	{
		QMessageBox msgBox;
		msgBox.setText( "No save file destination specified." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	MdiChild * mdiChild = static_cast<MdiChild*>( activeChild );
	mdiChild->initProgressBar();

	iAProgress* marCubProgress = iAProgress::New();
	iAProgress* stlWriProgress = iAProgress::New();
	connect( marCubProgress, SIGNAL( progress( int ) ), this, SLOT( updateMarProgress( int ) ) );
	connect( stlWriProgress, SIGNAL( progress( int ) ), this, SLOT( updateStlProgress( int ) ) );

	vtkSmartPointer<vtkMarchingCubes> moSurface = vtkSmartPointer<vtkMarchingCubes>::New();
	moSurface->AddObserver( vtkCommand::ProgressEvent, marCubProgress );
	moSurface->SetInputData( m_MOData.moImageDataList[iovMO->cb_Classes->currentIndex()] );
	moSurface->ComputeNormalsOn();
	moSurface->ComputeGradientsOn();
	moSurface->SetValue( 0, iovMO->dsb_IsoValue->value() );

	vtkSmartPointer<vtkSTLWriter> stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	stlWriter->AddObserver( vtkCommand::ProgressEvent, stlWriProgress );
	stlWriter->SetFileName( iovMO->le_StlPath->text().toStdString().c_str() );
	stlWriter->SetInputConnection( moSurface->GetOutputPort() );
	stlWriter->Write();
}

void dlg_FeatureScout::updateMarProgress(int i)
{
	MdiChild * mdiChild = static_cast<MdiChild*>( activeChild );
	mdiChild->updateProgressBar( i / 2  );
}

void dlg_FeatureScout::updateStlProgress( int i )
{
	MdiChild * mdiChild = static_cast<MdiChild*>( activeChild );
	mdiChild->updateProgressBar( 50 + i / 2 );
}

void CheckBounds( double color_out[3] )
{
	for ( int i = 0; i < 3; ++i )
	{
		if ( color_out[i] < 0 )
			color_out[i] = 0;
		if ( color_out[i] > 1.0 )
			color_out[i] = 1.0;
	}
}

void ColormapRGB( const double normal[3], double color_out[3] )
{
	for ( int i = 0; i < 3; ++i )
		color_out[i] = 0.5 + 0.5*normal[i];
	CheckBounds( color_out );
}

void ColormapCMY( const double normal[3], double color_out[3] )
{
	for ( int i = 0; i < 3; ++i )
		color_out[i] = 0.5 + 0.5*normal[i];
	CheckBounds( color_out );
	for ( int i = 0; i < 3; ++i )
		color_out[i] = 1 - color_out[i];
}

void ColormapCMYNormalized( const double normal[3], double color_out[3] )
{
	for ( int i = 0; i < 3; ++i )
		color_out[i] = 0.5 + 0.5*normal[i];
	CheckBounds( color_out );
	for ( int i = 0; i < 3; ++i )
		color_out[i] = 1 - color_out[i];
	vtkMath::Normalize( color_out );
}

void ColormapRGBNormalized( const double normal[3], double color_out[3] )
{
	for ( int i = 0; i < 3; ++i )
		color_out[i] = 0.5 + 0.5*normal[i];
	CheckBounds( color_out );
	vtkMath::Normalize( color_out );
}

void ColormapCMYAbsolute( const double normal[3], double color_out[3] )
{
	for ( int i = 0; i < 3; ++i )
		color_out[i] = fabs( normal[i] );
	CheckBounds( color_out );
	for ( int i = 0; i < 3; ++i )
		color_out[i] = 1 - color_out[i];
}

void ColormapRGBAbsolute( const double normal[3], double color_out[3] )
{
	for ( int i = 0; i < 3; ++i )
		color_out[i] = fabs( normal[i] );
	CheckBounds( color_out );
}

void ColormapCMYAbsoluteNormalized( const double normal[3], double color_out[3] )
{
	for ( int i = 0; i < 3; ++i )
		color_out[i] = fabs( normal[i] );
	CheckBounds( color_out );
	for ( int i = 0; i < 3; ++i )
		color_out[i] = 1 - color_out[i];
	vtkMath::Normalize( color_out );
}

void ColormapRGBAbsoluteNormalized( const double normal[3], double color_out[3] )
{
	for ( int i = 0; i < 3; ++i )
		color_out[i] = fabs( normal[i] );
	CheckBounds( color_out );
	vtkMath::Normalize( color_out );
}

void ColormapRGBHalfSphere( const double normal[3], double color_out[3] )
{
	double longest = std::max( normal[0], normal[1] );
	longest = std::max( normal[2], longest );
	if ( !longest ) longest = 0.00000000001;

	double oneVec[3] = { 1.0, 1.0, 1.0 };
	vtkMath::Normalize( oneVec );
	int sign = 1;
	if ( vtkMath::Dot( oneVec, normal ) < 0 )
		sign = -1;

	for ( int i = 0; i < 3; ++i )
		color_out[i] = 0.5 + 0.5*sign*normal[i];///longest;
	CheckBounds( color_out );
}

void dlg_FeatureScout::RenderingOrientation()
{
	//Turns off FLD scalar bar and updates polar plot view
	if ( m_scalarWidgetFLD != NULL )
	{
		m_scalarWidgetFLD->Off();
		this->updatePolarPlotColorScalar( chartTable );
	}

	iovPP->setWindowTitle( "Orientation Distribution Color Map" );

	// define color coding using hsv -> create color palette
	VTK_CREATE( vtkImageData, oi );
	oi->SetExtent( 0, 90, 0, 360, 0, 0 );
	oi->AllocateScalars( VTK_DOUBLE, 3 );

	for ( int theta = 0; theta < 91; theta++ )//theta
	{
		for ( int phi = 0; phi < 361; phi++ )//phi
		{
			double phi_rad = vtkMath::RadiansFromDegrees( (double) phi ),
				theta_rad = vtkMath::RadiansFromDegrees( (double) theta );
			double recCoord[3] = { sin( theta_rad ) * cos( phi_rad ),
				sin( theta_rad ) * sin( phi_rad ),
				cos( theta_rad ) };
			double *p = static_cast<double *>( oi->GetScalarPointer( theta, phi, 0 ) );
			vtkMath::Normalize( recCoord );
			ColormapRGB( recCoord, p );
			vtkMath::Normalize( recCoord );
			colormapsIndex[orientColormap->currentIndex()]( recCoord, p );
		}
	}

	// start rendering process with scalar values from the imagedata
	// rendering in 3D with color transfer function
	double backAlpha = 0.0, alpha = 0.5, red = 0.0, green = 0.0, blue = 0.0, backRGB[3];
	backRGB[0] = 0.0; backRGB[1] = 0.0; backRGB[2] = 0.0;
	int ip, it;

	// clear existing points
	this->oTF->RemoveAllPoints();
	this->cTF->RemoveAllPoints();
	//this->oTF->ClampingOff();
	//this->cTF->ClampingOff();

	this->oTF->AddPoint( 0, backAlpha );
	this->cTF->AddRGBPoint( 0, backRGB[0], backRGB[1], backRGB[2] );

	for ( int i = 0; i < this->objectNr; i++ )
	{
		if ( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
		{
			ip = qFloor( this->csvTable->GetValue( i, 13 ).ToDouble() );
			it = qFloor( this->csvTable->GetValue( i, 14 ).ToDouble() );
		}
		else
		{
			ip = qFloor( this->csvTable->GetValue( i, 16 ).ToDouble() );
			it = qFloor( this->csvTable->GetValue( i, 17 ).ToDouble() );
		}

		double *p = static_cast<double *>( oi->GetScalarPointer( it, ip, 0 ) );
		red = p[0]; green = p[1]; blue = p[2];
		this->oTF->AddPoint( i + 1, alpha );
		this->cTF->AddRGBPoint( i + 1, red, green, blue );
	}

	// update polar plot for orientation rendering
	this->pcPolarPlot->SetRenderWindow( NULL );

	// prepare the delaunay triangles
	VTK_CREATE( vtkDelaunay2D, del );
	VTK_CREATE( vtkPoints, points );
	double xx, yy, angle;

	// color array to save the colors for each point
	vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetNumberOfComponents( 3 );
	colors->SetName( "Colors" );

	for ( int phi = 0; phi < 360; phi++ )
	{
		for ( int theta = 0; theta < 91; theta++ )
		{
			angle = phi * M_PI / 180.0;
			xx = theta * cos( angle );
			yy = theta * sin( angle );
			points->InsertNextPoint( xx, yy, 0.0 );
			double *p = static_cast<double *>( oi->GetScalarPointer( theta, phi, 0 ) );
			unsigned char color[3];
			for ( unsigned int j = 0; j < 3; j++ )
				color[j] = static_cast<unsigned char>( 255.0 * p[j] );
#if (VTK_MAJOR_VERSION > 7 || (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION > 0))
			colors->InsertNextTypedTuple( color );
#else
			colors->InsertNextTupleValue(color);
#endif
		}
	}

	VTK_CREATE( vtkPolyData, inputPoly );
	inputPoly->SetPoints( points );
	del->SetInputData( inputPoly );
	del->Update();

	vtkPolyData *outputPoly = del->GetOutput();
	outputPoly->GetPointData()->SetScalars( colors );

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData( outputPoly );

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper( mapper );

	VTK_CREATE( vtkRenderer, renderer );
	renderer->SetBackground( 1, 1, 1 );

	VTK_CREATE( vtkRenderWindow, renW );
	renW->AddRenderer( renderer );
	renderer->AddActor( actor );

	// Projection grid and annotations
	this->drawPolarPlotMesh( renderer );
	this->drawAnnotations( renderer );

	vtkRenderWindowInteractor *renI = pcPolarPlot->GetInteractor();
	renI->SetRenderWindow( renW );
	pcPolarPlot->SetRenderWindow( renI->GetRenderWindow() );
	pcPolarPlot->GetRenderWindow()->Render();

	static_cast<MdiChild*>( activeChild )->updateViews();
	orientationColorMapSelection->show();
	this->orientColormap->show();
}

void dlg_FeatureScout::RenderingFLD()
{
	int numberOfBins;
	double range[2] = { 0.0, 0.0 };
	vtkDataArray* length;

	if ( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
	{
		numberOfBins = 8;
		iovPP->setWindowTitle( "Fibers Length Frequency Distribution" );
		length = vtkDataArray::SafeDownCast( this->csvTable->GetColumn( 18 ) );
	}
	else
	{
		numberOfBins = 3;
		iovPP->setWindowTitle( "Major Axis Length Frequency Distribution" );
		length = vtkDataArray::SafeDownCast( this->csvTable->GetColumn( 28 ) );
	}

	length->GetRange( range );
	if ( range[0] == range[1] )
		range[1] = range[0] + 1.0;

	double inc = ( range[1] - range[0] ) / (numberOfBins) * 1.001;
	double halfInc = inc / 2.0;

	VTK_CREATE( vtkFloatArray, extents );
	extents->SetName( "Length [um]" );
	extents->SetNumberOfTuples( numberOfBins );

	float *centers = static_cast<float *>( extents->GetVoidPointer( 0 ) );
	double min = range[0] - 0.0005*inc + halfInc;

	for ( int j = 0; j < numberOfBins; ++j )
		extents->SetValue( j, min + j*inc );

	VTK_CREATE( vtkIntArray, populations );
	populations->SetName( "Probability" );
	populations->SetNumberOfTuples( numberOfBins );
	int *pops = static_cast<int *>( populations->GetVoidPointer( 0 ) );

	for ( int k = 0; k < numberOfBins; ++k )
		pops[k] = 0;

	for ( vtkIdType j = 0; j < length->GetNumberOfTuples(); ++j )
	{
		double v = 0.0;
		length->GetTuple( j, &v );

		for ( int k = 0; k < numberOfBins; ++k )
		{
			if ( vtkMathUtilities::FuzzyCompare( v, double( centers[k] ), halfInc ) )
			{
				++pops[k];
				break;
			}
		}
	}

	VTK_CREATE( vtkTable, fldTable );
	fldTable->AddColumn( extents.GetPointer() );
	fldTable->AddColumn( populations.GetPointer() );

	//Create a transfer function mapping scalar value to color
	vtkSmartPointer<vtkColorTransferFunction> cTFun = vtkSmartPointer<vtkColorTransferFunction>::New();
	cTFun->SetColorSpaceToRGB();
	if ( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
	{
		cTFun->AddRGBPoint( range[0], 1.0, 0.6, 0.0 );	//orange
		cTFun->AddRGBPoint( extents->GetValue( 0 ) + halfInc, 1.0, 0.0, 0.0 ); //red
		cTFun->AddRGBPoint( extents->GetValue( 1 ) + halfInc, 1.0, 0.0, 1.0 ); //magenta
		cTFun->AddRGBPoint( extents->GetValue( 2 ) + halfInc, 0.0, 0.0, 1.0 ); //blue
		cTFun->AddRGBPoint( extents->GetValue( 7 ) + halfInc, 0.0, 1.0, 0.7 ); //cyan
	}
	else
	{
		cTFun->AddRGBPoint( range[0], 1.0, 0.6, 0.0 );	//orange
		cTFun->AddRGBPoint( extents->GetValue( 1 ) + halfInc, 1.0, 0.0, 1.0 ); //magenta
		cTFun->AddRGBPoint( extents->GetValue( 2 ) + halfInc, 0.0, 1.0, 0.7 ); //cyan
	}

	// start rendering process with colorlookuptable
	double backAlpha = 0.00005;
	double backRGB[3];
	backRGB[0] = 0.0;
	backRGB[1] = 0.0;
	backRGB[2] = 0.0;

	double red = 0.0;
	double green = 0.0;
	double blue = 0.0;
	double dcolor[3];
	int CID = 0;

	// clear existing points
	this->oTF->RemoveAllPoints();
	this->cTF->RemoveAllPoints();

	//this->oTF->ClampingOff();
	//this->cTF->ClampingOff();

	this->cTF->AddRGBPoint( 0, backRGB[0], backRGB[1], backRGB[2] );

	//alpha = colorLookupTable->GetAlpha();
	double alpha = 0.001;

	for ( int i = 0; i < this->objectNr; i++ )
	{
		double ll = length->GetTuple( i )[0];

		//colorLookupTable->GetColor(ll, dcolor);
		cTFun->GetColor( ll, dcolor );
		red = dcolor[0];
		green = dcolor[1];
		blue = dcolor[2];
		if ( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
		{
			if ( ll >= range[0] && ll < extents->GetValue( 0 ) + halfInc )
			{
				this->oTF->AddPoint( i + 1 - 0.5, 0.0 );
				this->oTF->AddPoint( i + 1 + 0.3, 0.0 );
				this->oTF->AddPoint( i + 1, 1.0 );
			}
			else if ( ll >= extents->GetValue( 0 ) + halfInc && ll < extents->GetValue( 1 ) + halfInc )
			{
				this->oTF->AddPoint( i + 1 - 0.5, 0.0 );
				this->oTF->AddPoint( i + 1 + 0.3, 0.0 );
				this->oTF->AddPoint( i + 1, 0.03 );
			}
			else if ( ll >= extents->GetValue( 1 ) + halfInc && ll < extents->GetValue( 2 ) + halfInc )
			{
				this->oTF->AddPoint( i + 1 - 0.5, 0.0 );
				this->oTF->AddPoint( i + 1 + 0.3, 0.0 );
				this->oTF->AddPoint( i + 1, 0.03 );
			}
			else if ( ll >= extents->GetValue( 2 ) + halfInc && ll < extents->GetValue( 5 ) + halfInc )
			{
				this->oTF->AddPoint( i + 1 - 0.5, 0.0 );
				this->oTF->AddPoint( i + 1 + 0.3, 0.0 );
				this->oTF->AddPoint( i + 1, 0.015 );
			}
			else if ( ll >= extents->GetValue( 5 ) + halfInc && ll <= extents->GetValue( 7 ) + halfInc )
			{
				this->oTF->AddPoint( i + 1 - 0.5, 0.0 );
				this->oTF->AddPoint( i + 1 + 0.3, 0.0 );
				this->oTF->AddPoint( i + 1, 1.0 );
			}
		}
		else
		{
			if ( ll >= range[0] && ll < extents->GetValue( 0 ) + halfInc )
			{
				this->oTF->AddPoint( i + 1 - 0.5, 0.0 );
				this->oTF->AddPoint( i + 1 + 0.3, 0.0 );
				this->oTF->AddPoint( i + 1, 0.5 );
			}
			else if ( ll >= extents->GetValue( 0 ) + halfInc && ll < extents->GetValue( 1 ) + halfInc )
			{
				this->oTF->AddPoint( i + 1 - 0.5, 0.0 );
				this->oTF->AddPoint( i + 1 + 0.3, 0.0 );
				this->oTF->AddPoint( i + 1, 0.5 );
			}
			else if ( ll >= extents->GetValue( 5 ) + halfInc && ll <= extents->GetValue( 2 ) + halfInc )
			{
				this->oTF->AddPoint( i + 1 - 0.5, 0.0 );
				this->oTF->AddPoint( i + 1 + 0.3, 0.0 );
				this->oTF->AddPoint( i + 1, 0.5 );
			}
		}
		this->cTF->AddRGBPoint( i + 1, red, green, blue );
		this->cTF->AddRGBPoint( i + 1 - 0.5, red, green, blue );
		this->cTF->AddRGBPoint( i + 1 + 0.3, red, green, blue );
	}

	this->orientationColorMapSelection->hide();
	this->orientColormap->hide();
	this->drawScalarBar( cTFun, this->raycaster->GetRenderer(), 1 );
	this->raycaster->update();

	// plot length distribution
	VTK_CREATE( vtkContextView, view );
	VTK_CREATE( vtkChartXY, chart );

	if ( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
	{
		chart->SetTitle( "Fibers Length Frequency Distribution" );
		chart->GetTitleProperties()->SetFontSize( 15 );
	}
	else
	{
		chart->SetTitle( "Major Axis Length Frequency Distribution" );
		chart->GetTitleProperties()->SetFontSize( 12 );
	}

	vtkPlot *plot = chart->AddPlot( vtkChartXY::BAR );
	plot->SetInputData( fldTable, 0, 1 );
	plot->GetXAxis()->SetTitle( "Length in microns" );
	plot->GetYAxis()->SetTitle( "Frequency" );
	view->GetScene()->AddItem( chart );
	view->SetInteractor( this->pcPolarPlot->GetInteractor() );
	this->pcPolarPlot->SetRenderWindow( view->GetRenderWindow() );
	pcPolarPlot->update();
}

void dlg_FeatureScout::ClassAddButton()
{
	int CountObject = 0;
	CountObject = pcChart->GetPlot( 0 )->GetSelection()->GetNumberOfTuples();

	if ( CountObject > 0 )
	{
		// class name and color
		int cid = classTreeModel->invisibleRootItem()->rowCount();
		QString cText = QString( "Class %1" ).arg( cid );

		// automatically select a predefined color
		// (from the list of colors defined in the list of SVG
		// color keyword names provided by the World Wide Web Consortium).
		//http://www.w3.org/TR/SVG/types.html#ColorKeywords
		if ( cid > 7 ) { cid = 1; }

		QColor cColor;

		switch ( cid )
		{
			case 1:
				cColor.setNamedColor( "cornflowerblue" ); break;
			case 2:
				cColor.setNamedColor( "darkorange" ); break;
			case 3:
				cColor.setNamedColor( "chartreuse" ); break;
			case 4:
				cColor.setNamedColor( "yellow" ); break;
			case 5:
				cColor.setNamedColor( "mediumvioletred" ); break;
			case 6:
				cColor.setNamedColor( "blue" ); break;
			case 7:
				cColor.setNamedColor( "green" ); break;
		}

		bool ok;

		// class name and color input when calling AddClassDialog.
		cText = dlg_editPCClass::getClassInfo( 0, "FeatureScout", cText, &cColor, &ok ).section( ',', 0, 0 );

		if ( ok ) // if input correctly
		{
			this->colorList.append( cColor );
			// get the root item from class tree
			QStandardItem *rootItem = classTreeModel->invisibleRootItem();
			QStandardItem *item;

			// create a first level child under rootItem as new class
			double percent = 100.0*CountObject / objectNr;
			QList<QStandardItem *> firstLevelItem = prepareRow( cText, QString( "%1" ).arg( CountObject ), QString::number( percent, 'f', 1 ) );
			firstLevelItem.first()->setData( cColor, Qt::DecorationRole );

			int ClassID = rootItem->rowCount();
			int objID = 0;
			QList<int> kIdx; // list to regist the selected index of object IDs in activeClassItem

			// add new class
			for ( int i = 0; i < CountObject; i++ )
			{
				// get objID from item->text()
				vtkVariant v = pcChart->GetPlot( 0 )->GetSelection()->GetVariantValue( i );
				objID = v.ToInt() + 1;	//fibre index starting at 1 not at 0

				//vtkVariant v = SelectedArr->GetComponent(i, 0);
				objID = this->activeClassItem->child( v.ToInt() )->text().toInt();

				if ( !kIdx.contains( v.ToInt() ) )
				{
					kIdx.prepend( v.ToInt() );
					selectedObjID.append( objID );

					// add item to the new class
					QString str = QString( "%1" ).arg( objID );
					item = new QStandardItem( str );
					firstLevelItem.first()->appendRow( item );

					// update Class_ID column, prepare values for LookupTable
					this->csvTable->SetValue( objID - 1, elementNr - 1, ClassID );
				}
			}

			// a simple check of the selections
			if ( kIdx.isEmpty() )
			{
				QMessageBox msgBox;
				msgBox.setText( "Selected objects are already classified, please make a new selection." );
				msgBox.setWindowTitle( "FeatureScout" );
				msgBox.exec();
				return;
			}

			if ( kIdx.count() != CountObject )
			{
				QMessageBox msgBox;
				msgBox.setText( "Selection Error, please make a new selection." );
				msgBox.setWindowTitle( "FeatureScout" );
				msgBox.exec();
				return;
			}

			// append the new class to class tree
			rootItem->appendRow( firstLevelItem );

			// count activeClassItem
			//int activeCount = this->activeClassItem->rowCount();

			// remove items from activeClassItem from table button to top, otherwise you would make a wrong delete
			for ( int i = 0; i < CountObject; i++ )
				this->activeClassItem->removeRow( kIdx.value( i ) );

			// update statistics for activeClassItem
			this->updateClassStatistics( this->activeClassItem );
			if ( this->activeClassItem->rowCount() == 0 && this->activeClassItem->index().row() != 0 )
			{
				// delete the activeItem
				int cID = this->activeClassItem->index().row();
				rootItem->removeRow( cID );
				this->colorList.removeAt( cID );
				//update Class_ID and lookupTable??

			}

			this->setActiveClassItem( firstLevelItem.first(), 1 );
			this->calculateElementTable();
			this->initElementTableModel();
			this->setupNewPcView();
			this->classTreeView->collapseAll();
			this->classTreeView->setCurrentIndex( firstLevelItem.first()->index() );
			this->SingleRendering();
			this->updatePolarPlotColorScalar( chartTable );

			//Updates scatter plot matrix when a class is added.
			if ( this->spmActivated && matrix != NULL )
			{
				bool retflag;
				QSharedPointer<QVector<uint>> selInd = QSharedPointer<QVector<uint>>(new QVector<uint>);
				*selInd = matrix->getSelection(); 
				applyClassSelection(retflag, selInd);
				if (retflag) return;


				
				
				//

				// matrix->UpdateColorInfo( classTreeModel, colorList );
				// matrix->SetClass2Plot( this->activeClassItem->index().row() );
				// matrix->GetAnnotationLink()->GetCurrentSelection()->RemoveAllNodes();
				// matrix->UpdateLayout();
			}
		}
	}
	else
	{
		QMessageBox msgBox;
		msgBox.setText( "No object was selected!" );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
	}
}

void dlg_FeatureScout::applyClassSelection(bool &retflag, QSharedPointer<QVector<uint>> selInd)
{
	retflag = true;
	double rgba[4];

	//QSharedPointer<QVector<uint>> selInd = QSharedPointer<QVector<uint>>(new QVector<uint>);
	bool returnflag;
	
	setSPMData(selInd, rgba, returnflag);
	if (retflag) return;




	spmApplyColorMap(rgba, selInd);
	retflag = false;
}

void dlg_FeatureScout::applyClassSelection(bool &retflag, vtkSmartPointer<vtkTable> classEntries) {
	


}

//copy all entries from a chart table to matrix
//with no selection?


void prepareTable(const int rowCount, const int colCount, QSharedPointer<QTableWidget> &spInput, vtkSmartPointer<vtkTable> &classEntries) {
	spInput->setColumnCount(colCount/*this->csvTable->GetNumberOfColumns()*/);
	//header (1 row) + entries
	spInput->setRowCount(rowCount/*this->csvTable->GetNumberOfRows()*/ + 1);
	for (int col = 0; col < colCount /*this->csvTable->GetNumberOfColumns()*/; ++col)
	{
		spInput->setItem(0, col, new QTableWidgetItem(classEntries->GetColumnName(col)));
	}

}

void dlg_FeatureScout::setSPMData(vtkSmartPointer<vtkTable> classEntries, double  rgba[4], bool &retflag) {
	setClassColour(rgba);
	QSharedPointer<QTableWidget> spInput = QSharedPointer<QTableWidget>(new QTableWidget);
	const int colCount = (int)classEntries->GetNumberOfColumns();//->GetNumberOfColumns();
	const int rowCount = (int)classEntries->GetNumberOfRows();
	prepareTable(rowCount, colCount, spInput, classEntries);
	


}

//set data in SPM selection to class
void dlg_FeatureScout::setSPMData(QSharedPointer<QVector<uint>> &selInd, double  rgba[4], bool &retflag)
{
	retflag = true;
	/**selInd = (this->matrix->getSelection());*/

	//indezes rausziehen selection indezes; 

	//TODO Save data in the selection 
	const uint entriesCount = (uint)selInd->length();
	setClassColour(rgba);
	QSharedPointer<QTableWidget> spInput = QSharedPointer<QTableWidget>(new QTableWidget);

	const int colCount = (int)this->csvTable->GetNumberOfColumns();
	const int rowCount = (int)this->csvTable->GetNumberOfRows();


	


	spInput->setColumnCount(colCount/*this->csvTable->GetNumberOfColumns()*/);
	//header (1 row) + entries
	spInput->setRowCount(entriesCount/*this->csvTable->GetNumberOfRows()*/ + 1);


	//set first colum Spalte ID setzen
	for (int col = 0; col < colCount /*this->csvTable->GetNumberOfColumns()*/; ++col)
	{
		spInput->setItem(0, col, new QTableWidgetItem(this->csvTable->GetColumnName(col)));
	}


	//set entrys for each row; 
	//TODO ersten eintrag mit 0 und dritten eintrag mit index 2 auswählen?? schauen ob das korrekt ist
	vtkSmartPointer<vtkAbstractArray> all_rowInd = csvTable->GetColumn(0);
	int cur_IndRow = 0;
	//TODO set label ID in first column? 
	bool containsRowInd = false;
	int rowSavingIndx = 1;
	QTableWidgetItem currItem; 


	

	for (auto const &curr_selIndx : *selInd) {

		//if row index is in selection index
		for (int row = 1; row < rowCount + 1; row++) {

			//skip first row
			cur_IndRow = all_rowInd->GetVariantValue(row - 1).ToInt() - 1;

			//if current selection index = rowIndex


			containsRowInd = ((int)curr_selIndx) == cur_IndRow;
			if (containsRowInd) {

				//adds each column entry to vtktable
				for (int col = 0; col < colCount; col++) {
					//current row index for saving sind
					//row index of spInput and QTableWidget are different!!

					vtkStdString csvValue = csvTable->GetValue(row - 1, col).ToString();
					spInput->setItem(rowSavingIndx, col, new QTableWidgetItem(csvValue.c_str()));

				}
				rowSavingIndx++;
				break;
			}

		}


	}




	//if scatterplot is active
	if (!iovSPM)
		return;
	//t/*his->matrix->clear(); */
	
	this->matrix->setData(&(*spInput));
	/*this->matrix->selectionModified(&(*selInd));*/
	this->spUpdateSPColumnVisibility();
	/*this->matrix->update();*/
	retflag = false;
}

void dlg_FeatureScout::setClassColour(double * rgba)
{
	rgba[0] = this->colorList.at(0).redF();
	rgba[1] = this->colorList.at(0).greenF();
	rgba[2] = this->colorList.at(0).blueF();
	const double alpha = 1;/*((double)this->colorList.at(0).alpha) / 255.0*/
	rgba[3] = alpha;
}

void dlg_FeatureScout::spmApplyColorMap(double  rgba[4], QSharedPointer<QVector<uint>> &selInd)
{
	double range[2];

	//csv table entry values -first columne min max for color transformation
	vtkDataArray *mmr = vtkDataArray::SafeDownCast(chartTable->GetColumn(0));
	mmr->GetRange(range);
	this->m_pointLUT = vtkSmartPointer<vtkLookupTable>::New();
	this->m_pointLUT->SetRange(range);
	this->m_pointLUT->SetTableRange(range);
	this->m_pointLUT->SetNumberOfTableValues(2);

	//set color for scatter plot and Renderer


	for (vtkIdType i = 0; i < 2; i++)
	{
		/*for (int i = 0; i < 4; ++i)*/

		this->m_pointLUT->SetTableValue(i, rgba);
	}
	this->m_pointLUT->Build();



	//wird das noch gebraucht
	this->matrix->setLookupTable(m_pointLUT, csvTable->GetColumnName(0));
	this->matrix->setSelectionColor(QColor(255, 40, 0, 1));

	//TODO show selection in color?
	
	
	this->matrix->update();
	
	
}

void dlg_FeatureScout::writeWisetex( QXmlStreamWriter *writer )
{
	// Write XML tags using WiseTex specification
	//check if it is a class item
	if ( classTreeModel->invisibleRootItem()->hasChildren() )
	{
		if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
		{
			writer->writeStartElement( "FibreClasses" ); //start FibreClasses tag

			for ( int i = 0; i < classTreeModel->invisibleRootItem()->rowCount(); i++ )
			{
				//Gets the fibre class
				QStandardItem* fc = classTreeModel->invisibleRootItem()->child( i, 1 );

				writer->writeStartElement( QString( "FibreClass-%1" ).arg( i ) ); //start FibreClass-n tag
				writer->writeTextElement( "CColor", QString( "0x00" ).append
										  ( QString( colorList.at( fc->index().row() ).name() ).remove( 0, 1 ) ) ); //CColor tag
				writer->writeTextElement( "NFibres", QString( fc->text() ) ); //NFibres tag

				writer->writeStartElement( QString( "Fibres" ) ); //Fibres tag

				for ( int j = 0; j < classTreeModel->invisibleRootItem()->child( i )->rowCount(); ++j )
				{
					writer->writeStartElement( QString( "Fibre-%1" ).arg( j + 1 ) ); //Fibre-n tag

					//Gets fibre features from csvTable
					writer->writeTextElement( "X1", QString( tableList[i]->GetValue( j, 1 ).ToString() ) );
					writer->writeTextElement( "Y1", QString( tableList[i]->GetValue( j, 2 ).ToString() ) );
					writer->writeTextElement( "Z1", QString( tableList[i]->GetValue( j, 3 ).ToString() ) );
					writer->writeTextElement( "X1", QString( tableList[i]->GetValue( j, 4 ).ToString() ) );
					writer->writeTextElement( "Y2", QString( tableList[i]->GetValue( j, 5 ).ToString() ) );
					writer->writeTextElement( "Z2", QString( tableList[i]->GetValue( j, 6 ).ToString() ) );
					writer->writeTextElement( "Phi", QString( tableList[i]->GetValue( j, 13 ).ToString() ) );
					writer->writeTextElement( "Theta", QString( tableList[i]->GetValue( j, 14 ).ToString() ) );
					writer->writeTextElement( "Dia", QString( tableList[i]->GetValue( j, 20 ).ToString() ) );
					writer->writeTextElement( "sL", QString( tableList[i]->GetValue( j, 18 ).ToString() ) );
					writer->writeTextElement( "cL", QString( tableList[i]->GetValue( j, 19 ).ToString() ) );
					writer->writeTextElement( "Surf", QString( tableList[i]->GetValue( j, 21 ).ToString() ) );
					writer->writeTextElement( "Vol", QString( tableList[i]->GetValue( j, 22 ).ToString() ) );

					writer->writeEndElement(); //end Fibre-n tag
				}
				writer->writeEndElement(); //end Fibres tag
				writer->writeEndElement(); //end FibreClass-n tag
			}
			writer->writeEndElement(); //end FibreClasses tag
		}
		else if ( filterID == INDIVIDUAL_PORE_VISUALIZATION )
		{
			writer->writeStartElement( "VoidClasses" ); //start FibreClasses tag

			for ( int i = 0; i < classTreeModel->invisibleRootItem()->rowCount(); i++ )
			{
				//Gets the fibre class
				QStandardItem* fc = classTreeModel->invisibleRootItem()->child( i, 1 );

				writer->writeStartElement( QString( "VoidClass-%1" ).arg( i ) ); //start VoidClass-n tag
				writer->writeTextElement( "CColor", QString( "0x00" ).append
										  ( QString( colorList.at( fc->index().row() ).name() ).remove( 0, 1 ) ) ); //CColor tag
				writer->writeTextElement( "NVoids", QString( fc->text() ) ); //NVoids tag

				writer->writeStartElement( QString( "Voids" ) ); //Voids tag

				for ( int j = 0; j < classTreeModel->invisibleRootItem()->child( i )->rowCount(); ++j )
				{
					writer->writeStartElement( QString( "Void-%1" ).arg( j + 1 ) ); //Void-n tag

					//Gets void properties from csvTable
					writer->writeTextElement( "Volume", QString( tableList[i]->GetValue( j, 21 ).ToString() ) );
					writer->writeTextElement( "dimX", QString( tableList[i]->GetValue( j, 13 ).ToString() ) );
					writer->writeTextElement( "dimY", QString( tableList[i]->GetValue( j, 14 ).ToString() ) );
					writer->writeTextElement( "dimZ", QString( tableList[i]->GetValue( j, 15 ).ToString() ) );
					writer->writeTextElement( "posX", QString( tableList[i]->GetValue( j, 18 ).ToString() ) );
					writer->writeTextElement( "posY", QString( tableList[i]->GetValue( j, 19 ).ToString() ) );
					writer->writeTextElement( "posZ", QString( tableList[i]->GetValue( j, 20 ).ToString() ) );
					//writer->writeTextElement("ShapeFactor", QString(tableList[i]->GetValue(j,22).ToString()));

					writer->writeEndElement(); //end Void-n tag
				}
				writer->writeEndElement(); //end Voids tag
				writer->writeEndElement(); //end VoidClass-n tag
			}
			writer->writeEndElement(); //end VoidClasses tag
		}
	}
}

void dlg_FeatureScout::CsvDVSaveButton()
{
	//Gets the selected rows out of elementTable
	QModelIndexList indexes = elementTableView->selectionModel()->selection().indexes();
	QList<ushort> rowList;
	QStringList inList;
	QList<QVariant> inPara;

	//Creates to checkboxes
	inList.append( QString( "$Save file" ) );
	inPara.append( false );
	inList.append( QString( "$Show histograms" ) );
	inPara.append( true );

	for ( int i = 0; i < indexes.count(); ++i )
	{
		//Ensures that indices are unique
		if ( rowList.contains( indexes.at( i ).row() ) )
			continue;
		rowList.append( indexes.at( i ).row() );

		QString columnName( this->elementTable->GetColumn( 0 )->GetVariantValue( rowList.at( i ) ).ToString().c_str() );
		columnName.remove( "Â" );

		inList.append( QString( "?Characteristic %1" ).arg( columnName ) );
		inList.append( QString( "#HistoMin for %1" ).arg( columnName ) );
		inList.append( QString( "#HistoMax for %1" ).arg( columnName ) );
		inList.append( QString( "#HistoBinNbr for %1" ).arg( columnName ) );

		inPara.append( this->elementTable->GetColumn( 1 )->GetVariantValue( rowList.at( i ) ).ToFloat() );
		inPara.append( this->elementTable->GetColumn( 2 )->GetVariantValue( rowList.at( i ) ).ToFloat() );
		inPara.append( 100 );
	}

	if ( rowList.count() == 0 )
	{
		QMessageBox msgBox;
		msgBox.setText( "No characteristic specified in the element explorer." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	dlg_commoninput dlg( this, "DistributionViewCSVSaveDialog", inList, inPara, NULL );

	if ( dlg.exec() == QDialog::Accepted && ( dlg.getCheckValue(0) == 2 || dlg.getCheckValue(1) == 2 ) )
	{
		QString filename;

		if ( dlg.getCheckValue(0) == 2 )
		{
			filename = QFileDialog::getSaveFileName( this, tr( "Save characteristic distributions" ), sourcePath, tr( "CSV Files (*.csv *.CSV)" ) );

			if ( filename.isEmpty() )
			{
				QMessageBox msgBox;
				msgBox.setText( "No save file destination specified." );
				msgBox.setWindowTitle( "FeatureScout" );
				msgBox.exec();
				return;
			}
		}

		this->m_dvContextView->GetScene()->ClearItems();

		//Sets up a chart matrix for the feature distribution charts
		vtkNew<vtkChartMatrix> distributionChartMatrix;
		distributionChartMatrix->SetSize( vtkVector2i( rowList.count() < 3 ? rowList.count() % 3 : 3, ceil( rowList.count() / 3.0 ) ) );
		distributionChartMatrix->SetGutter( vtkVector2f( 70.0, 70.0 ) );

		//Calculates histogram for each selected characteristic
		for ( int row = 0; row < rowList.count(); ++row )
		{
			double range[2] = { 0.0, 0.0 };
			vtkDataArray *length = vtkDataArray::SafeDownCast(
				this->tableList[this->activeClassItem->index().row()]->GetColumn( rowList.at( row ) ) );
			range[0] = dlg.getDblValue(4 * row + 3);
			range[1] = dlg.getDblValue(4 * row + 4);
			//length->GetRange(range);

			if ( range[0] == range[1] )
				range[1] = range[0] + 1.0;

			int numberOfBins = dlg.getDblValue(4 * row + 5);
			//int numberOfBins = dlg.getDblValue(row+2);
			//double inc = (range[1] - range[0]) / (numberOfBins) * 1.001; //test
			double inc = ( range[1] - range[0] ) / ( numberOfBins );
			double halfInc = inc / 2.0;

			VTK_CREATE( vtkFloatArray, extents );
			extents->SetName( "Value" );
			extents->SetNumberOfTuples( numberOfBins );

			float *centers = static_cast<float *>( extents->GetVoidPointer( 0 ) );
			//double min = range[0] - 0.0005*inc + halfInc;	//test
			double min = range[0] + halfInc;

			for ( int j = 0; j < numberOfBins; ++j )
				extents->SetValue( j, min + j * inc );;

			VTK_CREATE( vtkIntArray, populations );
			populations->SetName( "Probability" );
			populations->SetNumberOfTuples( numberOfBins );
			int *pops = static_cast<int *>( populations->GetVoidPointer( 0 ) );

			for ( int k = 0; k < numberOfBins; ++k )
				pops[k] = 0;

			for ( vtkIdType j = 0; j < length->GetNumberOfTuples(); ++j )
			{
				double v( 0.0 );
				length->GetTuple( j, &v );

				for ( int k = 0; k < numberOfBins; ++k )
				{
					//value lies between [center-halfInc & center+halfInc]
					if ( ( fabs( v - double( centers[k] ) ) ) <= halfInc )
					{
						//value lies between ]center-halfInc & center+halfInc[ and is assigned to class k
						if ( ( fabs( v - double( centers[k] ) ) ) < halfInc )
						{
							++pops[k];
							break;
						}
						// value = lower limit and is assigned to class k
						else if ( ( v - centers[k] ) == ( halfInc*-1.0 ) )
						{
							++pops[k];
							break;
						}
						// value = upper limit and is assigned to class k+1
						else if ( ( ( v - centers[k] ) == halfInc ) && ( v != range[1] ) )
						{
							++pops[k + 1];
							break;
						}
						// if value = range[1] assigned it to class k
						else if ( v == range[1] )
						{
							++pops[k];
							break;
						}
					}
				}
			}

			VTK_CREATE( vtkTable, disTable );
			disTable->AddColumn( extents.GetPointer() );
			disTable->AddColumn( populations.GetPointer() );

			//Writes csv file
			if ( dlg.getCheckValue(0) == 2 )
			{
				ofstream file( filename.toStdString().c_str(), std::ios::app );
				if ( file.is_open() )
				{
					vtkVariant tColNb, tRowNb, tVal;
					tColNb = disTable->GetNumberOfColumns();
					tRowNb = disTable->GetNumberOfRows();

					file << QString( "%1 Distribution of '%2'" )
						.arg( this->csvTable->GetColumnName( rowList.at( row ) ) )
						.arg( this->activeClassItem->text() ).toStdString()
						<< endl;

					file << "HistoBinID;" << "HistoBinCenter" << "Frequency;" << endl;

					for ( int row = 0; row < tRowNb.ToTypeUInt64(); ++row )
					{
						for ( int col = 0; col < tColNb.ToUnsignedShort(); ++col )
						{
							tVal = disTable->GetValue( row, col );
							switch ( col )
							{
								case 0:
									file << row + 1 << ";"
										<< std::setprecision( 20 ) << tVal.ToDouble() << ";";
									break;
								case 1:
									file << tVal.ToTypeUInt64() << endl;
									break;
							}
						}
					}
					file.close();
				}
			}

			//Creates chart for each selected characteristic
			if ( dlg.getCheckValue(1) == 2 )
			{
				vtkChartXY* chart = vtkChartXY::SafeDownCast( distributionChartMatrix
															  ->GetChart( vtkVector2i( row % ( rowList.count() < 3 ? rowList.count() % 3 : 3 ), row / 3 ) ) );
				//chart->SetBarWidthFraction(0.95);
				chart->GetTitleProperties()->SetFontSize( 18 );
				vtkPlot *plot = chart->AddPlot( vtkChartXY::BAR );
				plot->SetInputData( disTable, 0, 1 );
				plot->GetXAxis()->SetTitle( this->csvTable->GetColumnName( rowList.at( row ) ) );
				plot->GetYAxis()->SetTitle( "Frequency" );
				plot->GetXAxis()->GetLabelProperties()->SetFontSize( 19 );
				plot->GetYAxis()->GetLabelProperties()->SetFontSize( 19 );
				plot->GetXAxis()->GetTitleProperties()->SetFontSize( 19 );
				plot->GetYAxis()->GetTitleProperties()->SetFontSize( 19 );
			}
		}

		//Renders the distributionMatrix in a new dockWidget
		if ( dlg.getCheckValue(1) == 2 )
		{
			MdiChild * mdiChild = static_cast<MdiChild*>( activeChild );
			if ( !iovDV )
			{
				iovDV = new dlg_IOVDV( this );
				iovDV->setWindowTitle( QString( "Distribution View" ) );
				this->m_dvContextView->SetInteractor( iovDV->dockWidgetContents->GetInteractor() );
				iovDV->dockWidgetContents->SetRenderWindow( this->m_dvContextView->GetRenderWindow() );
				mdiChild->addDockWidget( Qt::RightDockWidgetArea, iovDV );
				iovDV->show();
			}

			if ( iovSPM && !iovMO || ( iovSPM && iovMO ) )
			{
				mdiChild->tabifyDockWidget( iovSPM, iovDV );
				iovDV->show();
				iovDV->raise();
			}
			else if ( !iovSPM && iovMO )
			{
				mdiChild->tabifyDockWidget( iovMO, iovDV );
				iovDV->show();
				iovDV->raise();
			}
			this->m_dvContextView->GetScene()->AddItem( distributionChartMatrix.GetPointer() );
			this->m_dvContextView->GetRenderWindow()->Render();
		}
	}
}

void dlg_FeatureScout::WisetexSaveButton()
{
	if ( !classTreeModel->invisibleRootItem()->hasChildren() )
	{
		QMessageBox msgBox;
		msgBox.setText( "No data available!" );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	//XML file save path
	QString filename = QFileDialog::getSaveFileName( this, tr( "Save File" ), sourcePath, tr( "XML Files (*.xml *.XML)" ) );
	if ( filename.isEmpty() )
	{
		QMessageBox msgBox;
		msgBox.setText( "No destination file was specified!" );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	QFile file( filename );
	if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		QMessageBox msgBox;
		msgBox.setText( "Could not open XML file." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	//Creates XML file
	QXmlStreamWriter stream( &file );
	stream.setAutoFormatting( true );
	stream.writeStartDocument();
	stream.writeStartElement( "WiseTex-XML" );
	writeWisetex( &stream );
	stream.writeEndElement();
	stream.writeEndDocument();
}

void dlg_FeatureScout::ClassSaveButton()
{
	if ( classTreeModel->invisibleRootItem()->rowCount() == 1 )
	{
		QMessageBox msgBox;
		msgBox.setText( "No classes was defined." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	QString filename = QFileDialog::getSaveFileName( this, tr( "Save File" ), sourcePath, tr( "XML Files (*.xml *.XML)" ) );
	if ( filename.isEmpty() )
	{
		QMessageBox msgBox;
		msgBox.setText( "No destination file was specified!" );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	QFile file( filename );
	if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		QMessageBox msgBox;
		msgBox.setText( "Could not open XML file." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	QXmlStreamWriter stream( &file );

	stream.setAutoFormatting( true );
	stream.writeStartDocument();
	stream.writeStartElement( IFVTag );
	stream.writeAttribute( VersionAttribute, "1.0" );
	stream.writeAttribute( CountAllAttribute, QString( "%1" ).arg( objectNr ) );

	for ( int i = 0; i < classTreeModel->invisibleRootItem()->rowCount(); i++ )
	{
		writeClassesAndChildren( &stream, classTreeModel->invisibleRootItem()->child( i ) );
	}

	stream.writeEndElement(); // ClassTree
	stream.writeEndDocument();
}

void dlg_FeatureScout::ClassLoadButton()
{
	// open xml file and get meta information
	QString filename = QFileDialog::getOpenFileName( this, tr( "Load xml file" ), sourcePath, tr( "XML Files (*.xml *.XML)" ) );
	if ( filename.isEmpty() )
	{
		QMessageBox msgBox;
		msgBox.setText( "Class load error: No source file was specified!" );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	QFile file( filename );
	if ( !file.open( QIODevice::ReadOnly ) )
	{
		QMessageBox msgBox;
		msgBox.setText( "Class load error: Could not open source xml file." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	// checking xml file correctness
	QXmlStreamReader checker( &file );
	checker.readNext();
	checker.readNext();
	if ( checker.name() == IFVTag )
	{
		// if the object number is not correct, stop the load process
		if ( checker.attributes().value( CountAllAttribute ).toString().toInt() != this->objectNr )
		{
			QMessageBox msgBox;
			msgBox.setText( "Class load error: Incorrect xml file for current dataset, please check." );
			msgBox.setWindowTitle( "FeatureScout" );
			msgBox.exec();
			checker.clear();
			return;
		}
	}
	else // incompatible xml file
	{
		QMessageBox msgBox;
		msgBox.setText( "Class load error: xml file incompatible with FeatureScout, please check." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		checker.clear();
		return;
	}
	// close the cheker puffer
	checker.clear();
	file.close();

	// start class laod process
	// clear classTree, colorList, TableList
	chartTable->DeepCopy( csvTable ); // reset charttable

	this->updateColumnNames();

	tableList.clear();
	tableList.push_back( chartTable );

	this->colorList.clear();
	this->classTreeModel->clear();

	// init header names for class tree model
	classTreeModel->setHorizontalHeaderItem( 0, new QStandardItem( "Class" ) );
	classTreeModel->setHorizontalHeaderItem( 1, new QStandardItem( "Count" ) );
	classTreeModel->setHorizontalHeaderItem( 2, new QStandardItem( "Percent" ) );

	int idxClass = 0;

	// create xml reader
	QStandardItem *rootItem = this->classTreeModel->invisibleRootItem();
	QStandardItem *activeItem;

	QFile readFile( filename );
	if ( !readFile.open( QIODevice::ReadOnly ) )
		return;

	QXmlStreamReader reader( &readFile );
	while ( !reader.atEnd() )
	{
		if ( reader.readNextStartElement() )
		{
			if ( reader.name() == ObjectTag )
			{
				QString label;
				if( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION)
					label = reader.attributes().value( LabelAttribute ).toString();
				else
					label = reader.attributes().value( LabelAttributePore ).toString();

				QStandardItem *item = new QStandardItem( label );

				// add objects to firstLevelClassItem
				activeItem->appendRow( item );

				// update Class_ID number in csvTable;
				this->csvTable->SetValue( label.toInt() - 1, this->elementNr, idxClass - 1 );
			}
			else if ( reader.name() == ClassTag )
			{
				// prepare the first level class items
				const QString name = reader.attributes().value( NameAttribute ).toString();
				const QString color = reader.attributes().value( ColorAttribute ).toString();
				const QString count = reader.attributes().value( CountAttribute ).toString();
				const QString percent = reader.attributes().value( PercentAttribute ).toString();

				QList<QStandardItem *> stammItem = this->prepareRow( name, count, percent );
				stammItem.first()->setData( QColor( color ), Qt::DecorationRole );
				this->colorList.append( QColor( color ) );
				rootItem->appendRow( stammItem );
				activeItem = rootItem->child( idxClass );
				idxClass++;
			}
		}
	}

	//upadate TableList
	if ( rootItem->rowCount() == idxClass )
	{
		for ( int i = 0; i < idxClass; i++ )
			this->recalculateChartTable( rootItem->child( i ) );
		this->setActiveClassItem( rootItem->child( 0 ), 0 );
	}
	else
	{
		QMessageBox msgBox;
		msgBox.setText( "Class load error: unclear class load process." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
	}
	reader.clear();
	readFile.close();
	if ( this->spmActivated )
	{
		// reinitialize spm
		MdiChild * mdiChild = static_cast<MdiChild*>( activeChild );
		mdiChild->removeDockWidget( iovSPM );
		this->layout()->removeWidget( iovSPM );
		delete iovSPM;
		iovSPM = nullptr;
		this->spmActivated = false;
		changeFeatureScout_Options( 6 );
	}
}

void dlg_FeatureScout::ClassDeleteButton()
{
	QStandardItem *rootItem = this->classTreeModel->invisibleRootItem();
	QStandardItem *stammItem = rootItem->child( 0 );
	if ( this->activeClassItem->index().row() == 0 )
	{
		QMessageBox msgBox;
		msgBox.setText( "You are trying to delete the unclassified class, please select another class." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return;
	}

	// define a list to sort the items in stammItem
	QList<int> list;
	// get Class_ID
	int cID = this->activeClassItem->index().row();
	int countActive = this->activeClassItem->rowCount();

	// append stamm item values to list
	for ( int i = 0; i < stammItem->rowCount(); i++ )
		list.append( stammItem->child( i )->text().toInt() );
	for ( int j = 0; j < countActive; j++ )
	{
		int v = this->activeClassItem->child( j )->text().toInt();
		// update Class_ID column, prepare values for LookupTable
		this->csvTable->SetValue( v - 1, elementNr - 1, 0 );
		// remove the object from selected IDs
		selectedObjID.removeOne( v );
		// append the deleted object IDs to list
		list.append( v );
	}

	// sort the new stamm list
	qSort( list );
	// give the values from list to stammitem
	stammItem->removeRows( 0, stammItem->rowCount() );
	for ( int k = 0; k < list.size(); k++ )
	{
		QStandardItem *item = new QStandardItem( QString( "%1" ).arg( list.at( k ) ) );
		stammItem->appendRow( item );
	}

	// update colorList
	this->colorList.removeAt( cID );

	// update statistics for activeClassItem
	this->updateClassStatistics( stammItem );

	// update tableList and setup activeClassItem
	this->setActiveClassItem( stammItem, 2 );
	this->setupNewPcView();

	// update element view
	this->calculateElementTable();
	this->initElementTableModel();

	// remove the deleted row item
	rootItem->removeRow( cID );
	this->SingleRendering();

	if ( this->spmActivated )
	{	//Updates SPM
		// TODO SPM
		// matrix->UpdateColorInfo( classTreeModel, colorList );
		// matrix->SetClass2Plot( this->activeClassItem->index().row() );
		// matrix->UpdateLayout();
	}
}


void dlg_FeatureScout::ScatterPlotButton()
{
	if ( this->spmActivated )
	{
		assert( !matrix );
		matrix = new iAQSplom();

		QTableWidget* spInput = new QTableWidget();
		spInput->setColumnCount(csvTable->GetNumberOfColumns());
		spInput->setRowCount(csvTable->GetNumberOfRows()+1);
		for (int col = 0; col < csvTable->GetNumberOfColumns(); ++col)
		{
			spInput->setItem(0, col, new QTableWidgetItem(csvTable->GetColumnName(col)));
		}
		for (int row = 1; row < csvTable->GetNumberOfRows()+1; ++row)
		{
			for (int col = 0; col < csvTable->GetNumberOfColumns(); ++col)
			{
				spInput->setItem(row, col, new QTableWidgetItem(csvTable->GetValue(row-1, col).ToString().c_str()) );
			}
		}

		// QVTKWidget setup and initialization
		if ( !iovSPM )
			return;
		//iovSPM->setWindowTitle( QString( "Scatter Plot Matrix" ) );

		//sp_qvtkWidget->setWindowFlags(Qt::CustomizeWindowHint|Qt::WindowCloseButtonHint);

		//Create vtkContextView and marry it with QVTKWidget
		//VTK_CREATE( vtkContextView, view );
		//view->SetInteractor( iovSPM->dockWidgetContents->GetInteractor() );
		//iovSPM->dockWidgetContents->SetRenderWindow( view->GetRenderWindow() );
		iovSPM->setWidget(matrix);

		// Add a ScatterPlotMatrix to the vtkContextView
		//view->GetScene()->AddItem( matrix );

		//PC-SPM-AnnotationLink-Wedding to get same red selection highlights.

		
		// pcChart->SetAnnotationLink( matrix->GetAnnotationLink() );
		//matrix->SetInput( spInput, this->filterID );

		//apply lookup table for scatter plot matrix
		matrix->setData(spInput);
		double range[2];
		
		//csv table einträge werte erste spalte min max für color transformation
		vtkDataArray *mmr = vtkDataArray::SafeDownCast(chartTable->GetColumn(0));
		mmr->GetRange(range);
		m_pointLUT = vtkSmartPointer<vtkLookupTable>::New();
		m_pointLUT->SetRange(range);
		m_pointLUT->SetTableRange(range);
		m_pointLUT->SetNumberOfTableValues(2);

		//set color for scatter plot and Renderer
		for (vtkIdType i = 0; i < 2; i++)
		{
			/*for (int i = 0; i < 4; ++i)*/
			double rgba[4] = { 0.5, 0.5, 0.5, 1.0 };
			m_pointLUT->SetTableValue(i, rgba);
		}
		m_pointLUT->Build();


		matrix->setLookupTable(m_pointLUT, csvTable->GetColumnName(0));
		matrix->setSelectionColor(QColor(255, 40, 0, 1));

		// TODO SPM

		// Updates scatter plot matrix with up to date (PC tree) information
		// about Class_ID and RGB color. IMPORTANT: Set UpdateColorInfo after SetInput()
		// matrix->UpdateColorInfo( classTreeModel, colorList );

		// If PC-Option 'Multi Rendering' is clicked a scatter plot matrix
		// with all (colored) plots is shown.
		// matrix->SetClass2Plot( this->activeClassItem->index().row() );

		// Scatter plot matrix settings
		if ( this->activeClassItem->rowCount() < 30 )
		{
			//matrix->SetPlotMarkerSize( 0, 3.0f );
			//matrix->SetPlotMarkerSize( 2, 10.0f );
		}
		//matrix->SetScatterPlotSelectedActiveColor( vtkColor4ub( 175, 238, 238, 130 ) );
		//matrix->SetScatterPlotSelectedRowColumnColor( vtkColor4ub( 0, 0, 0, 20 ) );
		//matrix->SetBackgroundColor( 1, vtkColor4ub( 235, 235, 235, 100 ) );
		//matrix->SetGutter( vtkVector2f( 30.0f, 30.0f ) );

		// Scatter plot matrix only shows features which are selected in PC-ElementTableModel.
		spUpdateSPColumnVisibility();

		// Creates a popup menu
		QMenu* popup1 = new QMenu( iovSPM );
		popup1->addAction( "Add Class" );
		//popup1->addAction( "Suggest Classification" );
		popup1->addSeparator();
		popup1->addAction( "Histograms On/Off" );
		popup1->addSeparator();
		popup1->addAction( "Subtraction Selection Mode" );
		popup1->addAction( "Polygon Selection Tool" );
		popup1->setStyleSheet( "font-size: 11px; background-color: #9B9B9B; border: 1px solid black;" );

		connect( popup1, SIGNAL( triggered( QAction* ) ), this, SLOT( spPopupSelection( QAction* ) ) );

		/*
		vtkEventQtSlotConnect *spConnections = vtkEventQtSlotConnect::New();
		// Gets right button release event (on a scatter plot).
		spConnections->Connect( iovSPM->dockWidgetContents->GetRenderWindow()->GetInteractor(),
								vtkCommand::RightButtonReleaseEvent,
								this,
								SLOT( spPopup( vtkObject*, unsigned long, void*, void*, vtkCommand* ) ),
								popup1, 1.0 );

		// Gets right button press event (on a scatter plot).
		spConnections->Connect( iovSPM->dockWidgetContents->GetRenderWindow()->GetInteractor(),
								vtkCommand::RightButtonPressEvent,
								this,
								SLOT( spBigChartMouseButtonPressed( vtkObject*, unsigned long, void*, void*, vtkCommand* ) ) );

		//Gets selection changed event (on scatterplot).
		spConnections->Connect( matrix,
								vtkCommand::SelectionChangedEvent,
								this,
								SLOT( spSelInformsPCChart( vtkObject*, unsigned long, void*, void*, vtkCommand* ) ), 0, 1.0 );
		*/
		//iovSPM->dockWidgetContents->show();

		//connects signal from SPM selection to PCView
		connect(matrix, SIGNAL(selectionModified(QVector<unsigned int> *)), this, SLOT(spSelInformsPCChart(QVector<unsigned int> *)) );
	}
}

void dlg_FeatureScout::spUpdateSPColumnVisibility()
{
	// Updates scatter plot matrix if a feature of PC-ElementTableModel is added or removed.
	if ( this->spmActivated )
	{
		// TODO SPM: only update matrix after all parameters visibility set?
		for ( int j = 0; j < elementNr; ++j )
		{
			matrix->setParameterVisibility( elementTable->GetValue( j, 0 ).ToString().c_str(),
				elementTableModel->item(j, 0)->checkState() == Qt::Checked );
		}
	}
}

void dlg_FeatureScout::spSelInformsPCChart(QVector<unsigned int> * selInds)

{
	vtkSmartPointer<vtkIdTypeArray> vtk_selInd = vtkSmartPointer<vtkIdTypeArray>::New();
	// If scatter plot selection changes Parallel Coordinates gets informed and updates.
	if ( this->spmActivated )
	{
		QCoreApplication::processEvents();
		//this->iovSPM->dockWidgetContents->update();
		// update selection in PC view!
		//set selection for pcView / chart
		int countSelection = selInds->length();
		//if (pcChart->GetPlot(0)->GetSelection()->GetMaxId)
		vtk_selInd->Allocate(countSelection);
		vtk_selInd->SetNumberOfValues(countSelection);
		int idx = 0;
		vtkVariant var_Idx = 0; 

		//current selection index
		long long curr_selInd;
		if (countSelection > 0) {
			

			for (auto ind:*selInds) {

				var_Idx = ind;
				curr_selInd = var_Idx.ToLongLong()  /*+1*/; 
				vtk_selInd->SetVariantValue(idx, curr_selInd);
				idx++; 
			}
		}
	
		this->pcChart->GetPlot(0)->SetSelection(vtk_selInd); 
		//this->pcChart->Update(); 
		this->pcView->Render();
		this->RealTimeRendering( pcChart->GetPlot( 0 )->GetSelection(), this->enableRealTimeRendering );
	}
}

void dlg_FeatureScout::spBigChartMouseButtonPressed( vtkObject * obj, unsigned long, void * client_data, void *, vtkCommand * command )
{
	// Gets the right mouse button press event for scatter plot matrix.
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast( obj );
	mousePressedPos[0] = iren->GetEventPosition()[0];
	mousePressedPos[1] = iren->GetEventPosition()[1];
}

void dlg_FeatureScout::spPopup( vtkObject * obj, unsigned long, void * client_data, void *, vtkCommand * command )
{
	// Gets the mouse button event for scatter plot matrix and opens a popup menu.
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast( obj );
	int* mouseReleasePos = iren->GetLastEventPosition();
	if ( mouseReleasePos[0] == mousePressedPos[0] && mouseReleasePos[1] == mousePressedPos[1] )
	{
		// Consume event so the interactor style doesn't get it
		command->AbortFlagOn();
		// Get popup menu
		QMenu* popupMenu = static_cast<QMenu*>( client_data );
		// Get event location
		int* sz = iren->GetSize();
		// Remember to flip y
		QPoint pt = QPoint( mouseReleasePos[0], sz[1] - mouseReleasePos[1] );
		// Map to global
		QPoint global_pt = popupMenu->parentWidget()->mapToGlobal( pt );
		// Show popup menu at global point
		popupMenu->popup( global_pt );
	}
	else
	{
		// Reset to original SPM popup
		QMenu* popupMenu = static_cast<QMenu*>( client_data );
		// Check if SPM or PC popup
		if ( popupMenu->actions().count() > 1 )
			popupMenu->actions().at( 1 )->setText( "Suggest Classification" );
	}
}

void dlg_FeatureScout::spPopupSelection( QAction *selection )
{
	// Function to handle the scatter plot matrix popup menu selection.
	if ( selection->text() == "Add Class" ) { ClassAddButton(); }
	// TODO SPM
	else if ( selection->text() == "Histograms On/Off" )
	{
		//matrix->ShowHideHistograms();
	}
	else if ( selection->text() == "Subtraction Selection Mode" )
	{
		//matrix->SetSelectionMode( vtkContextScene::SELECTION_SUBTRACTION );
		selection->setText( "Toggle Selection Mode" );
	}
	else if ( selection->text() == "Toggle Selection Mode" )
	{
		//matrix->SetSelectionMode( vtkContextScene::SELECTION_DEFAULT );
		selection->setText( "Subtraction Selection Mode" );
	}
	else if ( selection->text() == "Polygon Selection Tool" )
	{
		//matrix->setPolygonSelectionOn();
		selection->setText( "Rectangle Selection Tool" );
	}
	else if ( selection->text() == "Rectangle Selection Tool" )
	{
		//matrix->setRectangleSelectionOn();
		selection->setText( "Polygon Selection Tool" );
	}
	else if ( selection->text() == "Suggest Classification" )
	{
		bool ok;
		int i = QInputDialog::getInt( iovSPM, tr( "kMeans-Classification" ), tr( "Number of Classes" ), 3, 1, 7, 1, &ok );

		if ( ok )
		{

			//matrix->NumberOfClusters = i;
			//matrix->SetkMeansMode( true );
			selection->setText( "Accept Classification" );
		}
	}
	else if ( selection->text() == "Accept Classification" )
	{
		//autoAddClass( matrix->GetkMeansClusterCount() );
		selection->setText( "Suggest Classification" );
	}
}

void dlg_FeatureScout::autoAddClass( int NbOfClusters )
{
	if ( this->spmActivated )
	{
		QStandardItem *motherClassItem = this->activeClassItem;

		for ( int i = 1; i <= NbOfClusters; ++i )
		{
			// recieve the current selections from annotationlink
			// TODO SPM
			//vtkAbstractArray *SelArr = matrix->GetkMeansCluster( i )->GetNode( 0 )->GetSelectionList();
			int CountObject = 0; //  SelArr->GetNumberOfTuples();
			/*
			if ( CountObject > 0 )
			{
				// class name and color
				int cid = classTreeModel->invisibleRootItem()->rowCount();
				QString cText = QString( "Class %1" ).arg( cid );

				cid = cid % 8;	//rotate between prefedined colors only
				// automatically select a predefined color (http://www.w3.org/TR/SVG/types.html#ColorKeywords)
				QColor cColor;

				switch ( cid )
				{
					case 1:
						cColor.setNamedColor( "cornflowerblue" ); break;
					case 2:
						cColor.setNamedColor( "darkorange" ); break;
					case 3:
						cColor.setNamedColor( "chartreuse" ); break;
					case 4:
						cColor.setNamedColor( "yellow" ); break;
					case 5:
						cColor.setNamedColor( "mediumvioletred" ); break;
					case 6:
						cColor.setNamedColor( "blue" ); break;
					case 7:
						cColor.setNamedColor( "green" ); break;
				}

				this->colorList.append( cColor );

				// create a first level child under rootItem as new class
				double percent = 100.0*CountObject / objectNr;
				QList<QStandardItem *> firstLevelItem = prepareRow( cText, QString( "%1" ).arg( CountObject ), QString::number( percent, 'f', 1 ) );
				firstLevelItem.first()->setData( cColor, Qt::DecorationRole );

				QList<int> kIdx; // list to regist the selected index of object IDs in activeClassItem
				int objID = 0;
				// add new class
				for ( int i = 0; i < CountObject; ++i )
				{
					objID = SelArr->GetVariantValue( i ).ToInt();
					if ( !kIdx.contains( objID ) )
					{
						kIdx.prepend( objID );
						// add item to the new class
						firstLevelItem.first()->appendRow( new QStandardItem( QString( "%1" ).arg( objID ) ) );
						// update Class_ID column, prepare values for LookupTable
						this->csvTable->SetValue( objID - 1, elementNr - 1, cid );
					}
				}

				// append the new class to class tree
				classTreeModel->invisibleRootItem()->appendRow( firstLevelItem );

				// update chartTable
				this->setActiveClassItem( firstLevelItem.first(), 1 );
				this->classTreeView->setCurrentIndex( firstLevelItem.first()->index() );
			}
			else
			{
				QMessageBox msgBox;
				msgBox.setText( "No object was selected!" );
				msgBox.setWindowTitle( "FeatureScout" );
				msgBox.exec();
			}
			*/
		}

		//  Mother Class is empty after kMeans auto class added, therefore rows are removed
		classTreeModel->invisibleRootItem()->child( motherClassItem->index().row() )->
			removeRows( 0, classTreeModel->invisibleRootItem()->child( motherClassItem->index().row() )->rowCount() );

		// Update statistics for motherClassItem
		this->updateClassStatistics( classTreeModel->invisibleRootItem()->child( motherClassItem->index().row() ) );

		if ( this->activeClassItem->rowCount() == 0 && this->activeClassItem->index().row() != 0 )//new
		{
			// delete the activeItem
			int cID = this->activeClassItem->index().row();
			classTreeModel->invisibleRootItem()->removeRow( cID );
			this->colorList.removeAt( cID );
			//update Class_ID and lookupTable??
		}

		this->calculateElementTable();
		this->initElementTableModel();
		this->setupNewPcView();
		this->classTreeView->collapseAll();
		this->SingleRendering();
		this->updatePolarPlotColorScalar( chartTable );

		//Updates scatter plot matrix when a class is added.
		if ( matrix != NULL )
		{
			// TODO SPM
			//matrix->UpdateColorInfo( classTreeModel, colorList );
			//matrix->SetClass2Plot( this->activeClassItem->index().row() );
			//matrix->UpdateLayout();
		}
	}
}

void dlg_FeatureScout::classDoubleClicked( const QModelIndex &index )
{
	QStandardItem *item;
	// Gets right item from ClassTreeModel to remove AccesViolationError on item double click).
	// A class was clicked. (and no -1 row index - 'undefined' area click)
	if ( index.parent().row() == -1 && index.isValid() )
	{
		item = classTreeModel->item( index.row(), 0 );
		// Surpresses changeability items (class level) after dlg_editPCClass dialog.
		classTreeModel->itemFromIndex( index )->setEditable( false );
	}
	else if ( index.parent().row() != -1 && index.isValid() )
	{	// An item was clicked.
		// Surpresses changeability of items (single fiber level) after dlg_editPCClass dialog.
		classTreeModel->itemFromIndex( index )->setEditable( false );
		return;
	}
	else	// An 'undifined area' was clicked.
		return;

	this->pcWidget->setEnabled( true );

	if ( item->hasChildren() )
	{
		bool ok;
		QColor old_cColor = this->colorList.at( index.row() );
		QString old_cText = item->text();
		QColor new_cColor = old_cColor;
		QString new_cText = old_cText;
		new_cText = dlg_editPCClass::getClassInfo( 0, "Class Explorer", new_cText, &new_cColor, &ok ).section( ',', 0, 0 );

		if ( ok && ( old_cText.compare( new_cText ) != 0 || new_cColor != old_cColor ) )
		{
			this->colorList[index.row()] = new_cColor;
			item->setText( new_cText );
			item->setData( new_cColor, Qt::DecorationRole );
			this->SingleRendering();
			if ( this->spmActivated )
			{
				// Update Scatter Plot Matrix when another class than the active is selected.
				if ( matrix)
				{
					// TODO SPM
					//matrix->SetSelection(annLink->GetCurrentSelection());
					// matrix->UpdateColorInfo( classTreeModel, colorList );
					// matrix->SetClass2Plot( this->activeClassItem->index().row() );
					// matrix->UpdateLayout();
				}
			}
		}
	}
}

void dlg_FeatureScout::classClicked( const QModelIndex &index )
{
	//Turns off FLD scalar bar updates polar plot view
	if ( m_scalarWidgetFLD )
	{
		m_scalarWidgetFLD->Off();
		this->updatePolarPlotColorScalar( chartTable );
	}

	this->orientationColorMapSelection->hide();
	this->orientColormap->hide();

	// Gets right Item from ClassTreeModel
	QStandardItem *item;

	// checks if click on class 'level'
	if ( index.parent().row() == -1 )
		item = classTreeModel->item( index.row(), 0 );
	else
		item = classTreeModel->itemFromIndex( index );

	// check if unclassified class is empty
	if ( this->classTreeModel->invisibleRootItem()->child( 0 ) == item && item->rowCount() == 0 )
	{
		QMessageBox msgBox;
		msgBox.setText( "Unclassified class contains no object more, please make another selection." );
		msgBox.setWindowTitle( "Parallel Coordinates View" );
		msgBox.exec();
		return;
	}
	//MOD kMeans
	if ( item->rowCount() == 0 && item->parent()->index() == this->classTreeModel->invisibleRootItem()->index() )
		//if(item->rowCount() == 0)
	{
		QMessageBox msgBox;
		msgBox.setText( "This class contains no object more, please make another selection or delete the class." );
		msgBox.setWindowTitle( "Parallel Coordinates View" );
		msgBox.exec();
		return;
	}
	// for first level class //children = class
	if ( item->hasChildren() )
	{
		if ( this->activeClassItem != item )
		{
			this->setActiveClassItem( item );
			this->calculateElementTable();
			this->setupNewPcView();
			this->updatePolarPlotColorScalar( chartTable );
			this->SingleRendering();
			this->initElementTableModel();

			if ( this->spmActivated )
			{
				// Update Scatter Plot Matrix when another class than the active is selected.
				if ( matrix )
				{


					/*
					
					int id = item->index().row();
					chartTable->ShallowCopy( tableList[id] );
					*/
					/*int id = item->index().row();*/
					/*chartTable->ShallowCopy(tableList[id]);*/
					//da sind die eintrgäe drin!! 
					int u = item->rowCount();
					int u1 = item->columnCount(); 


					QList< QStandardItem* > items = item->takeRow(1);
					
					QStandardItem* xxx  = items.at(0);
					auto x = xxx->takeColumn(0); 
					/*	at(0)->data().toString(); */
					/*item->take; 
					item_data.
					*//*item->data()*/
					/*what is class  what are childs?? cvsTable are all entries, but not  */
					/*matrix->clear();*/ 
					//old entries are saved in matrix?? 
					
					QSharedPointer<QVector<uint>> selInd = QSharedPointer<QVector<uint>>(new QVector<uint>);
					bool returnFlag = false;
					/*this->applyClassSelection(returnFlag, nullptr, );*/
					if (returnFlag) return;
				}
			}
		}
		if ( !classRendering )
		{
			classRendering = true;
			this->SingleRendering();
			this->initElementTableModel();
		}
	}
	else // for single object selection
	{
		// update ParallelCoordinates
		if ( item->parent() != this->activeClassItem )
		{
			this->setActiveClassItem( item->parent() );
			this->calculateElementTable();
			this->setupNewPcView();
			this->updatePolarPlotColorScalar( chartTable );
		}

		// update ParallelCoordinates view selection
		int sID = item->index().row();

		// Fill selection with IDs
		vtkSmartPointer<vtkIdTypeArray> testArr = vtkSmartPointer<vtkIdTypeArray>::New();
		testArr->SetName( "Label" );
		testArr->InsertNextValue( sID );

		// Lines below are commented cause of selection error of a single void object in a new defined class
		//if(!spmActivated)
		this->pcChart->GetPlot( 0 )->SetSelection( testArr );
		//else
		//this->matrix->GetAnnotationLink()->GetCurrentSelection()->GetNode(0)->SetSelectionList(testArr);

		pcView->ResetCamera();
		pcView->Render();

		// update elementTableView
		this->initElementTableModel( sID );
		int oID = item->text().toInt();
		this->SingleRendering( oID );
		elementTableView->update();

		if ( this->spmActivated )
		{
			// Update Scatter Plot Matrix when another class than the active is selected.
			//set ID selection im feature scout
			if ( matrix )
			{
				
				//matrix->SetClass2Plot( this->activeClassItem->index().row() );
				//matrix->UpdateLayout();
			}
		}
	}
}

double dlg_FeatureScout::calculateOpacity( QStandardItem *item )
{
	// chart opacity dependence of number of objects
	// for multi rendering
	if ( item == this->classTreeModel->invisibleRootItem() )
	{
		if ( objectNr < 1000 )
			return 1.0;
		if ( objectNr < 3000 )
			return 0.8;
		if ( objectNr < 10000 )
			return 0.6;
		return 0.5;
	}
	// for single class rendering
	else if ( item->hasChildren() )
	{
		int n = item->rowCount();

		if ( n < 1000 )
			return 1.0;
		if ( n < 3000 )
			return 0.8;
		if ( n < 10000 )
			return 0.6;
		if ( n < 20000 )
			return 0.2;
		return 0.4;
	}
	// default opacity:
	return 1.0;
}

void dlg_FeatureScout::writeClassesAndChildren( QXmlStreamWriter *writer, QStandardItem *item )
{
	// check if it is a class item
	if ( item->hasChildren() )
	{
		writer->writeStartElement( ClassTag );
		writer->writeAttribute( NameAttribute, item->text() );

		QString color = QString( colorList.at( item->index().row() ).name() );

		writer->writeAttribute( ColorAttribute, color );
		writer->writeAttribute( CountAttribute, classTreeModel->invisibleRootItem()->child( item->index().row(), 1 )->text() );
		writer->writeAttribute( PercentAttribute, classTreeModel->invisibleRootItem()->child( item->index().row(), 2 )->text() );
		for ( int i = 0; i < item->rowCount(); i++ )
		{
			writer->writeStartElement( ObjectTag );
			for ( int j = 0; j < elementNr; j++ )
			{
				vtkVariant v = csvTable->GetValue( item->child( i )->text().toInt() - 1, j );
				QString str = QString::fromUtf8( v.ToUnicodeString().utf8_str() ).trimmed();

				if ( eleString.count()>0 )
				{
					writer->writeAttribute( eleString.at( j ), str );
				}
				else
				{
					vtkVariant v1 = elementTable->GetValue( j, 0 );
					QString str1 = QString::fromUtf8( v1.ToUnicodeString().utf8_str() ).trimmed();
					writer->writeAttribute( str1, str );
				}
			}
			writer->writeEndElement(); // end object tag
		}
		writer->writeEndElement(); // end class tag
	}
}


void dlg_FeatureScout::setActiveClassItem( QStandardItem* item, int situ )
{
	// check once more, if its really a class item
	if ( !item->hasChildren() )
		return;

	if ( situ == 0 )	// class clicked
	{
		// setActiveClassItem
		this->activeClassItem = item;
		// make sure when a class is added, at the same time the tableList should also be updated

		// reload the class table to chartTable
		int id = item->index().row();
		chartTable->ShallowCopy( tableList[id] );

		// reset selection to NULL
		// TODO SPM
		//if ( this->spmActivated )
			//this->matrix->GetAnnotationLink()->GetCurrentSelection()->RemoveAllNodes();
	}
	else if ( situ == 1 )	// add class
	{
		// recalculate the old active class
		this->recalculateChartTable( this->activeClassItem );

		// calculate the new class table and set up chartTable
		this->recalculateChartTable( item );

		// set active class
		this->activeClassItem = item;

		// reload the class table to chartTable
		int id = item->index().row();
		chartTable->ShallowCopy( tableList[id] );

		// reset selection to NULL
		// TODO SPM
		// if ( this->spmActivated )
			// this->matrix->GetAnnotationLink()->GetCurrentSelection()->RemoveAllNodes();
	}
	else if ( situ == 2 )	// delete class
	{
		// merge the deleted class table to stamm table
		this->recalculateChartTable( item );
		chartTable->ShallowCopy( tableList[0] );

		// delete the old activeClassItem in tableList
		tableList.removeAt( this->activeClassItem->index().row() );

		this->activeClassItem = item;
	}
	else
		return;
}

void dlg_FeatureScout::recalculateChartTable( QStandardItem *item )
{
	if ( !item->hasChildren() )
		return;

	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
	vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName( chartTable->GetColumnName( 0 ) );
	table->AddColumn( arr );
	for ( int i = 1; i < elementNr - 1; i++ )
	{
		vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
		arrX->SetName( chartTable->GetColumnName( i ) );
		table->AddColumn( arrX );
	}
	vtkSmartPointer<vtkIntArray> arrI = vtkSmartPointer<vtkIntArray>::New();
	arrI->SetName( chartTable->GetColumnName( elementNr - 1 ) );
	table->AddColumn( arrI );

	int oCount = item->rowCount();
	table->SetNumberOfRows( oCount );

	int csvID = 0;
	for ( int j = 0; j < oCount; j++ )
	{
		csvID = item->child( j )->text().toInt();
		vtkSmartPointer<vtkVariantArray> arr = vtkSmartPointer<vtkVariantArray>::New();
		csvTable->GetRow( csvID - 1, arr );
		table->SetRow( j, arr );
	}

	// if item already exists
	int itemID = item->index().row();
	if ( itemID + 1 <= tableList.size() )
	{
		// add the new acitve class table to tableList
		tableList.insert( itemID, table );
		// delete the old active class table
		tableList.removeAt( itemID + 1 );
	}
	else
	{
		// add the new table to the end of the tableList
		// maka a copy to chartTable
		tableList.push_back( table );
		chartTable->ShallowCopy( tableList[item->index().row()] );
	}
}

void dlg_FeatureScout::updateColumnNames()
{
	eleString.clear();
	eleString = getNamesOfObjectCharakteristics( false );
	const char* element;
	for ( int i = 0; i < elementNr; i++ )
	{
		QByteArray bA = eleString.at( i ).toUtf8();
		element = bA.constData();
		this->chartTable->GetColumn( i )->SetName( element );
	}
}

void dlg_FeatureScout::deletePcViewPointer()
{
	if ( this->pcChart != 0 )
	{
		pcChart->Delete();
		pcChart = 0;
	}
	if ( this->pcView != 0 )
	{
		pcView->Delete();
		pcView = 0;
	}
}

void dlg_FeatureScout::updateLookupTable( double alpha )
{
	int lutNum = colorList.length();
	lut->SetNumberOfTableValues( lutNum );
	lut->Build();
	for ( int i = 0; i < lutNum; i++ )
		lut->SetTableValue( i,
		colorList.at( i ).red() / 255.0,
		colorList.at( i ).green() / 255.0,
		colorList.at( i ).blue() / 255.0,
		colorList.at( i ).alpha() / 255.0 );

	lut->SetRange( 0, lutNum - 1 );
	lut->SetAlpha( alpha );
}

void dlg_FeatureScout::EnableBlobRendering()
{
	if ( !OpenBlobVisDialog() )
		return;
	iABlobCluster* blob = NULL;
	// check if that class is already visualized; if yes, update the existing blob:
	if ( blobMap.contains( this->activeClassItem->text() ) )
	{
		blob = blobMap[this->activeClassItem->text()];
	}
	else
	{
		blob = new iABlobCluster;
		blobManager->AddBlob( blob );
		blobMap.insert( this->activeClassItem->text(), blob );
	}
	blobManager->UpdateBlobSettings( blob );

	// single class rendering to blob view
	double bounds[6];
	raycaster->GetImageDataBounds( bounds );
	double dims[3] = { bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4] };
	double maxDim = dims[0] >= dims[1] ? dims[0] : dims[1];
	maxDim = dims[2] >= maxDim ? dims[2] : maxDim;

	QVector<FeatureInfo> objects;
	for ( int i = 0; i < chartTable->GetNumberOfRows(); i++ )
	{
		FeatureInfo fi;
		int index = chartTable->GetValue( i, 0 ).ToInt();

		if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
		{
			fi.x1 = chartTable->GetValue( i, 1 ).ToDouble();
			fi.y1 = chartTable->GetValue( i, 2 ).ToDouble();
			fi.z1 = chartTable->GetValue( i, 3 ).ToDouble();
			fi.x2 = chartTable->GetValue( i, 4 ).ToDouble();
			fi.y2 = chartTable->GetValue( i, 5 ).ToDouble();
			fi.z2 = chartTable->GetValue( i, 6 ).ToDouble();
			fi.diameter = chartTable->GetValue( i, 20 ).ToDouble();
		}
		else
		{
			fi.x1 = chartTable->GetValue( i, 1 ).ToDouble();
			fi.y1 = chartTable->GetValue( i, 2 ).ToDouble();
			fi.z1 = chartTable->GetValue( i, 3 ).ToDouble();
			fi.x2 = chartTable->GetValue( i, 4 ).ToDouble();
			fi.y2 = chartTable->GetValue( i, 5 ).ToDouble();
			fi.z2 = chartTable->GetValue( i, 6 ).ToDouble();
			fi.diameter = chartTable->GetValue( i, 23 ).ToDouble();;
		}
		objects.append( fi );
	}

	// set right objecttype (for the labeling)
	if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
		blob->SetObjectType( "Fibers" );
	else
		blob->SetObjectType( "Voids" );

	blob->SetLabelScale( 10.0 * maxDim / 100.0 );
	blob->SetCluster( objects );

	// set color
	QColor color = colorList.at( activeClassItem->index().row() );
	blob->GetSurfaceProperty()->SetColor( color.redF(), color.greenF(), color.blueF() );
	blob->SetName( activeClassItem->text() );
	const double count = activeClassItem->rowCount();
	const double percentage = 100.0*count / objectNr;
	blob->SetStats( count, percentage );
	blobManager->Update();
}

void dlg_FeatureScout::DisableBlobRendering()
{
	// delete blob for class
	if ( blobMap.contains( this->activeClassItem->text() ) )
	{
		iABlobCluster* blob = blobMap.take( this->activeClassItem->text() );
		blobManager->RemoveBlob( blob );
		delete blob;
	}
}

void dlg_FeatureScout::showContextMenu( const QPoint &pnt )
{
	QStandardItem *item = this->classTreeModel->itemFromIndex( this->classTreeView->currentIndex() );

	if ( !item || item->column() > 0 )
		return;

	QList<QAction *> actions;
	if ( this->classTreeView->indexAt( pnt ).isValid() )
	{
		if ( item->hasChildren() )
		{
			actions.append( this->blobRendering );
			actions.append( this->blobRemoveRendering );
			actions.append( this->saveBlobMovie );
			actions.append( this->objectAdd );
		}
		else
		{
			if ( item->parent()->index().row() != 0 )
				actions.append( this->objectDelete );
		}
	}
	if ( actions.count() > 0 )
		QMenu::exec( actions, this->classTreeView->mapToGlobal( pnt ) );
}

void dlg_FeatureScout::addObject()
{
	QMessageBox msgBox;
	msgBox.setText( "Adding an object to the active class, needs to be done." );
	msgBox.setWindowTitle( "Parallel Coordinates View" );
	msgBox.exec();
	// checking Class_ID, delete the object from the formerly class, update the corresponding class Table in Table list
	// adding the new object to the active class, update class table, reordering the label id...
}

void dlg_FeatureScout::deleteObject()
{
	QStandardItem *item = this->classTreeModel->itemFromIndex( this->classTreeView->currentIndex() );
	if ( item->hasChildren() )
		return;

	// if the parent item is the unclassified item
	if ( item->parent()->index() == this->classTreeModel->invisibleRootItem()->child( 0 )->index() )
	{
		QMessageBox msgBox;
		msgBox.setText( "Object in unclassified class could not be deleted." );
		msgBox.setWindowTitle( "Parallel Coordinates View" );
		msgBox.exec();
		return;
	}

	if ( item->parent()->rowCount() == 1 )
	{
		this->ClassDeleteButton();
	}
	else
	{
		int oID = item->text().toInt();
		selectedObjID.removeOne( oID );
		this->csvTable->SetValue( oID - 1, elementNr - 1, 0 );

		QStandardItem *sItem = this->classTreeModel->invisibleRootItem()->child( 0 );
		QStandardItem *newItem = new QStandardItem( QString( "%1" ).arg( oID ) );

		if ( oID == 1 || sItem->rowCount() == 0 )
		{
			sItem->insertRow( 0, newItem );
		}
		else if ( oID > sItem->child( sItem->rowCount() - 1 )->text().toInt() )
		{
			sItem->appendRow( newItem );
		}
		else
		{
			int i = 0;
			while ( oID > sItem->child( i )->text().toInt() )
			{
				i++;
			}
			sItem->insertRow( i, newItem );
		}
		this->updateClassStatistics( sItem );
		this->recalculateChartTable( sItem );

		QStandardItem *rItem = item->parent();
		rItem->removeRow( item->index().row() );
		this->updateClassStatistics( rItem );
		this->recalculateChartTable( rItem );
		this->setActiveClassItem( rItem, 0 );
	}
}

void dlg_FeatureScout::updateClassStatistics( QStandardItem *item )
{
	QStandardItem *rootItem = this->classTreeModel->invisibleRootItem();

	if ( item->hasChildren() )
	{
		rootItem->child( item->index().row(), 1 )->setText( QString( "%1" ).arg( item->rowCount() ) );
		double percent = 100.0*item->rowCount() / objectNr;
		rootItem->child( item->index().row(), 2 )->setText( QString::number( percent, 'f', 1 ) );
	}
	else
	{
		if ( item->index() == rootItem->child( 0 )->index() )
		{
			rootItem->child( 0, 1 )->setText( "0" );
			rootItem->child( 0, 2 )->setText( "0.00" );
		}
		//MOD kMeans
		else if ( item->rowCount() == 0 )
		{
			rootItem->child( item->index().row(), 1 )->setText( "0" );
			rootItem->child( item->index().row(), 2 )->setText( "0.00" );
		}
	}
}

void dlg_FeatureScout::addLogMsg( const QString &str )
{
	static_cast<MdiChild*>( activeChild )->addMsg( str );
}

int dlg_FeatureScout::calcOrientationProbability( vtkTable *t, vtkTable *ot )
{
	// compute the probability distribution of orientations
	ot->Initialize();
	int maxF = 0;
	double fp, ft;
	int ip, it, tt, length;

	for ( int i = 0; i < gThe; i++ )
	{
		vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
		arr->SetNumberOfValues( gPhi );
		ot->AddColumn( arr );
		for ( int j = 0; j < gPhi; j++ )
			ot->SetValue( j, i, 0 );
	}

	length = t->GetNumberOfRows();

	for ( int k = 0; k < length; k++ )
	{
		if ( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
		{
			fp = t->GetValue( k, 13 ).ToDouble() / PolarPlotPhiResolution;
			ft = t->GetValue( k, 14 ).ToDouble() / PolarPlotThetaResolution;
		}
		else
		{
			fp = t->GetValue( k, 16 ).ToDouble() / PolarPlotPhiResolution;
			ft = t->GetValue( k, 17 ).ToDouble() / PolarPlotThetaResolution;
		}

		ip = vtkMath::Round( fp );
		it = vtkMath::Round( ft );

		//if(ip > 360 || ip < 0 || it < 0 || it > 91)
		//	QString( "error computed phi" );

		if ( ip == gPhi )
			ip = 0;

		tt = ot->GetValue( ip, it ).ToInt();
		ot->SetValue( ip, it, tt + 1 );

		if ( maxF < tt + 1 )
			maxF = tt + 1;
	}
	return maxF;
}

void dlg_FeatureScout::updateObjectOrientationID( vtkTable *table )
{
	double fp, ft;
	int ip, it, tt;

	for ( int k = 0; k < this->objectNr; k++ )
	{
		if ( this->filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
		{
			fp = this->csvTable->GetValue( k, 13 ).ToDouble() / PolarPlotPhiResolution;
			ft = this->csvTable->GetValue( k, 14 ).ToDouble() / PolarPlotThetaResolution;
		}
		else
		{
			fp = this->csvTable->GetValue( k, 16 ).ToDouble() / PolarPlotPhiResolution;
			ft = this->csvTable->GetValue( k, 17 ).ToDouble() / PolarPlotThetaResolution;
		}

		ip = vtkMath::Round( fp );
		it = vtkMath::Round( ft );

		if ( ip == gPhi )
			ip = 0;

		tt = table->GetValue( ip, it ).ToInt();
		this->ObjectOrientationProbabilityList.append( tt );
	}
}

void dlg_FeatureScout::drawAnnotations( vtkRenderer *renderer )
{
	// annotations for phi
	vtkIdType numPoints = 12 + 6;
	double re = 30.0;

	VTK_CREATE( vtkPolyData, poly );
	VTK_CREATE( vtkPoints, pts );
	VTK_CREATE( vtkStringArray, nameArray );

	nameArray->SetName( "name" );
	pts->SetNumberOfPoints( numPoints );
	double x[3];

	for ( vtkIdType i = 0; i < 12; i++ )
	{
		double phi = i * re * M_PI / 180.0;
		double rx = 100.0;

		if ( i < 2 )
			rx = 95.0;

		x[0] = rx * cos( phi );
		x[1] = rx * sin( phi );
		x[2] = 0.0;

		pts->SetPoint( i, x );
		vtkVariant v = i * re;
		nameArray->InsertNextValue( v.ToString() );
	}

	// annotation for theta
	for ( vtkIdType i = 12; i < numPoints; i++ )
	{
		double phi = 270.0 * M_PI / 180.0;
		double rx = ( numPoints - i ) * 15.0;
		x[0] = rx * cos( phi );
		x[1] = rx * sin( phi );
		x[2] = 0.0;

		pts->SetPoint( i, x );
		vtkVariant v = rx;
		nameArray->InsertNextValue( v.ToString() );
	}
	poly->SetPoints( pts );
	poly->GetPointData()->AddArray( nameArray );

	VTK_CREATE( vtkDynamic2DLabelMapper, mapper );
	mapper->SetInputData( poly );
	mapper->SetLabelFormat( "%s" );
	mapper->SetLabelModeToLabelFieldData();
	mapper->SetFieldDataName( "name" );
	mapper->GetLabelTextProperty()->SetColor( 0.0, 0.0, 0.0 );
	mapper->GetLabelTextProperty()->SetFontSize( 16 );

	VTK_CREATE( vtkActor2D, actor );
	actor->SetMapper( mapper );
	renderer->AddActor( actor );
}

void dlg_FeatureScout::drawPolarPlotMesh( vtkRenderer *renderer )
{
	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();

	double xx, yy;
	double re = 15.0;
	int ap = 25;
	int at = 7;

	vtkSmartPointer<vtkStructuredGrid> sGrid = vtkSmartPointer<vtkStructuredGrid>::New();
	sGrid->SetDimensions( at, ap, 1 );
	int anzP = sGrid->GetNumberOfPoints();

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	points->Allocate( anzP );

	for ( int i = 0; i < ap; i++ )
	{
		double phi = i*re*M_PI / 180.0;

		for ( int j = 0; j < at; j++ )
		{
			double rx = j*re;
			xx = rx*cos( phi );
			yy = rx*sin( phi );
			points->InsertNextPoint( xx, yy, 0.0 );
		}
	}

	// add points to grid
	sGrid->SetPoints( points );

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

	// using vtkStructuredGridGeometryFilter
	vtkSmartPointer<vtkStructuredGridGeometryFilter> plane =
		vtkSmartPointer<vtkStructuredGridGeometryFilter>::New();
	plane->SetExtent( 0, at, 0, ap, 0, 0 );
	plane->SetInputData( sGrid );
	mapper->SetInputConnection( plane->GetOutputPort() );

	actor->SetMapper( mapper );
	actor->GetProperty()->SetColor( 1, 1, 1 );
	actor->GetProperty()->SetRepresentationToWireframe();
	actor->GetProperty()->SetLineWidth( 0.1 );
	actor->GetProperty()->SetOpacity( 0.2 );
	renderer->AddActor( actor );
}


void dlg_FeatureScout::drawScalarBar( vtkScalarsToColors *lut, vtkRenderer *renderer, int RenderType )
{
	// Default: RenderTpye = 0
	// 1		RenderFLD

	vtkScalarBarRepresentation* sbr;

	switch ( RenderType )
	{
		case 0:
			m_scalarWidgetPP = vtkSmartPointer<vtkScalarBarWidget>::New();
			m_scalarBarPP = vtkSmartPointer<vtkScalarBarActor>::New();
			m_scalarBarPP->SetLookupTable( lut );
			m_scalarBarPP->GetLabelTextProperty()->SetColor( 0, 0, 0 );
			m_scalarBarPP->GetTitleTextProperty()->SetColor( 0, 0, 0 );
			m_scalarBarPP->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
			m_scalarBarPP->SetTitle( "Frequency" );
			m_scalarBarPP->SetNumberOfLabels( 5 );
			m_scalarWidgetPP->SetInteractor( this->pcPolarPlot->GetInteractor() );
			m_scalarWidgetPP->SetScalarBarActor( m_scalarBarPP );
			m_scalarWidgetPP->SetEnabled( true );
			m_scalarWidgetPP->SetRepositionable( true );
			m_scalarWidgetPP->SetResizable( true );
			m_scalarWidgetPP->GetScalarBarActor()->SetTextPositionToSucceedScalarBar();
			sbr = vtkScalarBarRepresentation::SafeDownCast( m_scalarWidgetPP->GetRepresentation() );
			sbr->SetPosition( 0.88, 0.14 );
			sbr->SetPosition2( 0.11, 0.80 );
			break;

		case 1:
			m_scalarWidgetFLD = vtkSmartPointer<vtkScalarBarWidget>::New();
			m_scalarBarFLD = vtkSmartPointer<vtkScalarBarActor>::New();
			m_scalarBarFLD->SetLookupTable( lut );
			m_scalarBarFLD->GetLabelTextProperty()->SetColor( 0, 0, 0 );
			m_scalarBarFLD->GetTitleTextProperty()->SetColor( 0, 0, 0 );
			m_scalarBarFLD->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
			m_scalarBarFLD->SetTitle( "Length in microns" );
			m_scalarBarFLD->SetNumberOfLabels( 9 );
			m_scalarWidgetFLD->SetInteractor( raycaster->GetInteractor() );
			m_scalarWidgetFLD->SetScalarBarActor( m_scalarBarFLD );
			m_scalarWidgetFLD->SetEnabled( true );
			m_scalarWidgetFLD->SetRepositionable( true );
			m_scalarWidgetFLD->SetResizable( true );
			m_scalarWidgetFLD->GetScalarBarActor()->SetTextPositionToSucceedScalarBar();
			sbr = vtkScalarBarRepresentation::SafeDownCast( m_scalarWidgetFLD->GetRepresentation() );
			sbr->SetPosition( 0.93, 0.20 );
			sbr->SetPosition2( 0.07, 0.80 );
			break;

		default:
			break;
	}
}

void dlg_FeatureScout::createPolarPlotLookupTable( vtkLookupTable *lut )
{
	// set up an individual color table for LookupTable
	double rgb[3];
	double h = 0.0;
	double s = 1.0;
	double v = 1.0;
	int nv = 1800;

	lut->SetNumberOfTableValues( nv + 1 );

	for ( int i = 0; i < nv + 1; i++ )
	{
		h = ( nv - i ) / 3600.0;
		vtkMath::HSVToRGB( h, s, v, &rgb[0], &rgb[1], &rgb[2] );
		lut->SetTableValue( i, rgb[0], rgb[1], rgb[2] );
	}

	vtkMath::HSVToRGB( 0.6, 1.0, 1.0, &rgb[0], &rgb[1], &rgb[2] );
	lut->SetTableValue( 0, rgb[0], rgb[1], rgb[2] );
}

void dlg_FeatureScout::createFLDODLookupTable( vtkLookupTable *lut, int Num )
{
	if ( Num > 9 )
		return;

	lut->SetNumberOfTableValues( Num );
	lut->SetTableValue( 0, 1.0, 0.0, 0.0 );
	lut->SetTableValue( 1, 1.0, 0.549, 0.0 );
	lut->SetTableValue( 2, 1.0, 1.0, 0.0 );
	lut->SetTableValue( 3, 0.0, 1.0, 0.0 );
	lut->SetTableValue( 4, 0.0, 1.0, 1.0 );
	lut->SetTableValue( 5, 0.0, 0.0, 1.0 );
	lut->SetTableValue( 6, 1.0, 0.0, 1.0 );
	lut->SetTableValue( 7, 0.5, 0.0, 0.5 );

	if ( Num == 9 )
		lut->SetTableValue( 8, 0.5, 0.0, 0.0 );
}

void dlg_FeatureScout::setupPolarPlotView( vtkTable *it )
{
	iovPP->setWindowTitle( "Polar Plot View" );
	this->pcPolarPlot->SetRenderWindow( NULL );
	double xx, yy, zz, phi;

	// construct delaunay triangulation
	delaunay = vtkDelaunay2D::New();

	// create point sets
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

	// calculate object probability and save it to a table
	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
	this->pcMaxC = this->calcOrientationProbability( it, table );

	// update the object probability ID list
	this->updateObjectOrientationID( table );

	// Create a transfer function mapping scalar value to color
	vtkSmartPointer<vtkColorTransferFunction> cTFun = vtkSmartPointer<vtkColorTransferFunction>::New();

	//cold-warm-map
	//cTFun->AddRGBPoint(   0, 1.0, 1.0, 1.0 );
	//cTFun->AddRGBPoint(   1, 0.0, 1.0, 1.0 );
	//cTFun->AddRGBPoint(  pcMaxC, 1.0, 0.0, 1.0 );

	//heatmap
	cTFun->AddRGBPoint( 0.0, 0.74, 0.74, 0.74, 0.1, 0.0 );					//gray
	cTFun->AddRGBPoint( pcMaxC * 1 / 9.0, 0.0, 0.0, 1.0, 0.1, 0.0 );		//blue
	cTFun->AddRGBPoint( pcMaxC * 4 / 9.0, 1.0, 0.0, 0.0, 0.1, 0.0 );		//red
	cTFun->AddRGBPoint( pcMaxC * 9 / 9.0, 1.0, 1.0, 0.0, 0.1, 0.0 );		//yellow
	//cTFun->AddRGBPoint(   pcMaxC, 1.0, 1.0, 1.0 );						//white

	// color array to save the colors for each point
	vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetNumberOfComponents( 3 );
	colors->SetName( "Colors" );

	for ( int x = 0; x < gThe; x++ )
	{
		double rx = x*PolarPlotThetaResolution;

		for ( int y = 0; y < gPhi; y++ )
		{
			phi = y*PolarPlotPhiResolution*M_PI / 180.0;
			xx = rx*cos( phi );
			yy = rx*sin( phi );
			zz = table->GetValue( y, x ).ToDouble();

			if ( this->draw3DPolarPlot )
				points->InsertNextPoint( xx, yy, zz );
			else
				points->InsertNextPoint( xx, yy, 0.0 );

			double dcolor[3];

			cTFun->GetColor( zz, dcolor );
			unsigned char color[3];

			for ( unsigned int j = 0; j < 3; j++ )
				color[j] = static_cast<unsigned char>( 255.0 * dcolor[j] );

#if (VTK_MAJOR_VERSION > 7 || (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION > 0))
			colors->InsertNextTypedTuple( color );
#else
			colors->InsertNextTupleValue( color );
#endif
		}
	}

	// Add the grid points to a polydata object
	vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
	inputPolyData->SetPoints( points );
	inputPolyData->GetPointData()->SetScalars( colors );

	// initialize and triagulate the grid points for one time
	// the triangulated net will be reused later to get the polydata
	delaunay->SetInputData( inputPolyData );
	delaunay->Update();

	vtkPolyData* outputPolyData = delaunay->GetOutput();

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData( outputPolyData );

	mapper->SetLookupTable( cTFun );

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper( mapper );
	actor->GetProperty()->LightingOff();

	VTK_CREATE( vtkRenderer, renderer );
	renderer->SetBackground( 1, 1, 1 );

	VTK_CREATE( vtkRenderWindow, renW );
	renW->AddRenderer( renderer );
	renderer->AddActor( actor );

	vtkRenderWindowInteractor *renI = pcPolarPlot->GetInteractor();
	renI->SetRenderWindow( renW );
	pcPolarPlot->SetRenderWindow( renI->GetRenderWindow() );

	// Projection grid
	this->drawPolarPlotMesh( renderer );
	this->drawAnnotations( renderer );
	this->drawScalarBar( cTFun, renderer, 0 );
	pcPolarPlot->GetRenderWindow()->Render();
}

void dlg_FeatureScout::updatePolarPlotColorScalar( vtkTable *it )
{
	iovPP->setWindowTitle( "Polar Plot View" );
	this->pcPolarPlot->SetRenderWindow( NULL );
	double xx, yy, zz, phi;

	// calculate object probability and save it to a table
	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

	//this->setupPolarPlotResolution(2.0);
	int maxF = this->calcOrientationProbability( it, table );

	// Create a transfer function mapping scalar value to color
	vtkSmartPointer<vtkColorTransferFunction> cTFun = vtkSmartPointer<vtkColorTransferFunction>::New();
	//cold-warm-map
	//cTFun->AddRGBPoint(   0, 1.0, 1.0, 1.0 );
	//cTFun->AddRGBPoint(   1, 0.0, 1.0, 1.0 );
	//cTFun->AddRGBPoint(  maxF, 1.0, 0.0, 1.0 );

	//heatmap
	cTFun->AddRGBPoint( 0.0, 0.74, 0.74, 0.74, 0.1, 0.0 );				//gray
	cTFun->AddRGBPoint( maxF*1.0 / 9.0, 0.0, 0.0, 1.0, 0.1, 0.0 );		//blue
	cTFun->AddRGBPoint( maxF*4.0 / 9.0, 1.0, 0.0, 0.0, 0.1, 0.0 );		//red
	cTFun->AddRGBPoint( maxF*9.0 / 9.0, 1.0, 1.0, 0.0, 0.1, 0.0 );		//yellow
	//cTFun->AddRGBPoint(   maxF, 1.0, 1.0, 1.0 );						//white

	// color array to save the colors for each point
	vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetNumberOfComponents( 3 );
	colors->SetName( "Colors" );

	for ( int x = 0; x < gThe; x++ )
	{
		double rx = x*PolarPlotThetaResolution;

		for ( int y = 0; y < gPhi; y++ )
		{
			phi = y*PolarPlotPhiResolution*M_PI / 180.0;
			xx = rx*cos( phi );
			yy = rx*sin( phi );
			zz = table->GetValue( y, x ).ToDouble();

			points->InsertNextPoint( xx, yy, 0.0 );

			double dcolor[3];
			zz = table->GetValue( y, x ).ToDouble();

			cTFun->GetColor( zz, dcolor );

			unsigned char color[3];

			for ( unsigned int j = 0; j < 3; j++ )
				color[j] = static_cast<unsigned char>( 255.0 * dcolor[j] );

#if (VTK_MAJOR_VERSION > 7 || (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION > 0))
			colors->InsertNextTypedTuple( color );
#else
			colors->InsertNextTupleValue( color );
#endif
		}
	}

	// Add the grid points to a polydata object
	vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
	inputPolyData->SetPoints( points );
	inputPolyData->GetPointData()->SetScalars( colors );

	delaunay->SetInputData( inputPolyData );
	delaunay->Update();

	vtkPolyData* outputPolyData = delaunay->GetOutput();

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData( outputPolyData );

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper( mapper );
	actor->GetProperty()->LightingOff();

	VTK_CREATE( vtkRenderer, renderer );
	renderer->SetBackground( 1, 1, 1 );

	VTK_CREATE( vtkRenderWindow, renW );
	renW->AddRenderer( renderer );
	renderer->AddActor( actor );

	vtkRenderWindowInteractor *renI = pcPolarPlot->GetInteractor();
	renI->SetRenderWindow( renW );
	pcPolarPlot->SetRenderWindow( renI->GetRenderWindow() );

	// Projection grid and annotations
	this->drawPolarPlotMesh( renderer );
	this->drawAnnotations( renderer );
	this->drawScalarBar( cTFun, renderer );
	pcPolarPlot->GetRenderWindow()->Render();
}

void dlg_FeatureScout::setupPolarPlotResolution( float grad )
{
	this->gPhi = vtkMath::Floor( 360.0 / grad );
	this->gThe = vtkMath::Floor( 90.0 / grad );
	this->PolarPlotPhiResolution = 360.0 / gPhi;
	this->PolarPlotThetaResolution = 90.0 / gThe;
	gThe = gThe + 1;
}

int dlg_FeatureScout::OpenBlobVisDialog()
{
	QStringList inList = ( QStringList()
						   << tr( "^Range:" )
						   << tr( "$Blob body:" )
						   << tr( "$Use Depth peeling:" )
						   << tr( "^Blob opacity [0,1]:" )
						   << tr( "$Silhouettes:" )
						   << tr( "^Silhouettes opacity [0,1]:" )
						   << tr( "$3D labels:" )
						   << tr( "$Smart overlapping:" )
						   << tr( "^Separation distance (if smart overlapping):" )
						   << tr( "$Smooth after smart overlapping:" )
						   << tr( "$Gaussian blurring of the blob:" )
						   << tr( "*Gaussian blur variance:" )
						   << tr( "*Dimension X" )
						   << tr( "*Dimension Y" )
						   << tr( "*Dimension Z" )
						   );
	QList<QVariant> inPara;
	iABlobCluster* blob = NULL;
	if ( blobMap.contains( this->activeClassItem->text() ) )
		blob = blobMap[this->activeClassItem->text()];

	inPara
		<< tr( "%1" ).arg( blob ? blob->GetRange() : blobManager->GetRange() )
		<< tr( "%1" ).arg( blob ? blob->GetShowBlob() : blobManager->GetShowBlob() )
		<< tr( "%1" ).arg( blobManager->GetUseDepthPeeling() )
		<< tr( "%1" ).arg( blob ? blob->GetBlobOpacity() : blobManager->GetBlobOpacity() )
		<< tr( "%1" ).arg( blob ? blob->GetSilhouette() : blobManager->GetSilhouettes() )
		<< tr( "%1" ).arg( blob ? blob->GetSilhouetteOpacity() : blobManager->GetSilhouetteOpacity() )
		<< tr( "%1" ).arg( blob ? blob->GetLabel() : blobManager->GetLabeling() )
		<< tr( "%1" ).arg( blobManager->OverlappingIsEnabled() )
		<< tr( "%1" ).arg( blobManager->GetOverlapThreshold() )
		<< tr( "%1" ).arg( blob ? blob->GetSmoothing() : blobManager->GetSmoothing() )
		<< tr( "%1" ).arg( blobManager->GetGaussianBlur() )
		<< tr( "%1" ).arg( blob ? blob->GetGaussianBlurVariance() : blobManager->GetGaussianBlurVariance() )
		<< tr( "%1" ).arg( blob ? blob->GetDimensions()[0] : blobManager->GetDimensions()[0] )
		<< tr( "%1" ).arg( blob ? blob->GetDimensions()[1] : blobManager->GetDimensions()[1] )
		<< tr( "%1" ).arg( blob ? blob->GetDimensions()[2] : blobManager->GetDimensions()[2] );
	dlg_commoninput dlg( this, "Blob rendering preferences", inList, inPara, NULL );

	if ( dlg.exec() == QDialog::Accepted )
	{
		int i = 0;
		blobManager->SetRange               ( dlg.getDblValue(i++) );
		blobManager->SetShowBlob            ( dlg.getCheckValue(i++) != 0 );
		blobManager->SetUseDepthPeeling     ( dlg.getCheckValue(i++) );
		blobManager->SetBlobOpacity         ( dlg.getDblValue(i++) );
		blobManager->SetSilhouettes         ( dlg.getCheckValue(i++) != 0 );
		blobManager->SetSilhouetteOpacity   ( dlg.getDblValue(i++) );
		blobManager->SetLabeling            ( dlg.getCheckValue(i++) != 0 );
		blobManager->SetOverlappingEnabled  ( dlg.getCheckValue(i++) != 0 );
		blobManager->SetOverlapThreshold    ( dlg.getDblValue(i++) );
		blobManager->SetSmoothing           ( dlg.getCheckValue(i++) );
		blobManager->SetGaussianBlur        ( dlg.getCheckValue(i++) );
		blobManager->SetGaussianBlurVariance( dlg.getIntValue(i++) );

		int dimens[3];
		dimens[0] = dlg.getIntValue(i++);
		dimens[1] = dlg.getIntValue(i++);
		dimens[2] = dlg.getIntValue(i++);
		blobManager->SetDimensions( dimens );
		return 1;
	}
	else
		return 0;
}


QStringList dlg_FeatureScout::getNamesOfObjectCharakteristics( bool withUnit )
{
	// manually define new table elements
	// using unicode to define the unit
	QString micro = QChar( 0x00B5 );
	QString ud = QChar( 0x00B0 );
	QString h2 = QChar( 0x00B2 );
	QString h3 = QChar( 0x00B3 );

	QString udegree = QString( "" );
	QString micro1 = QString( "" );
	QString micro2 = QString( "" );
	QString micro3 = QString( "" );

	if ( withUnit )
	{
		udegree = QString( "[%1]" ).arg( ud );
		micro1 = QString( "[%1m]" ).arg( micro );
		micro2 = QString( "[%1m%2]" ).arg( micro ).arg( h2 );
		micro3 = QString( "[%1m%2]" ).arg( micro ).arg( h3 );
	}

	QStringList eleString;

	//Names of the fiber characteristics
	if ( filterID == INDIVIDUAL_FIBRE_VISUALIZATION )
	{
		eleString.append( "Label" );								// 0
		eleString.append( QString( "X1%1" ).arg( micro1 ) );		// 1
		eleString.append( QString( "Y1%1" ).arg( micro1 ) );		// 2
		eleString.append( QString( "Z1%1" ).arg( micro1 ) );		// 3
		eleString.append( QString( "X2%1" ).arg( micro1 ) );		// 4
		eleString.append( QString( "Y2%1" ).arg( micro1 ) );		// 5
		eleString.append( QString( "Z2%1" ).arg( micro1 ) );		// 6
		eleString.append( "a11" );									// 7
		eleString.append( "a22" );									// 8
		eleString.append( "a33" );									// 9
		eleString.append( "a12" );									// 10
		eleString.append( "a13" );									// 11
		eleString.append( "a23" );									// 12
		eleString.append( QString( "phi%1" ).arg( udegree ) );		// 13
		eleString.append( QString( "theta%1" ).arg( udegree ) );	// 14
		eleString.append( QString( "Xm%1" ).arg( micro1 ) );		// 15
		eleString.append( QString( "Ym%1" ).arg( micro1 ) );		// 16
		eleString.append( QString( "Zm%1" ).arg( micro1 ) );		// 17
		if ( withUnit )
		{
			eleString.append( QString( "StraightLength%1" ).arg( micro1 ) );	// 18
			eleString.append( QString( "CurvedLength%1" ).arg( micro1 ) );		// 19
		}
		else
		{
			eleString.append( QString( "sL%1" ).arg( micro1 ) );				// 18
			eleString.append( QString( "cL%1" ).arg( micro1 ) );				// 19
		}
		eleString.append( QString( "Diameter%1" ).arg( micro1 ) );				// 20
		eleString.append( QString( "Surface%1" ).arg( micro2 ) );				// 21
		eleString.append( QString( "Volume%1" ).arg( micro3 ) );				// 22

		if ( withUnit )
		{
			eleString.append( "SperatedFiber" );								// 23
			eleString.append( "CurvedFiber" );									// 24
		}
		else
		{
			eleString.append( "sFiber" );										// 23
			eleString.append( "cFiber" );										// 24
		}
	}
	//Names of the pore charakteristics
	else
	{
		eleString.append( "LabelId" );									// 0
		eleString.append( QString( "X1%1" ).arg( micro1 ) );			// 1
		eleString.append( QString( "Y1%1" ).arg( micro1 ) );			// 2
		eleString.append( QString( "Z1%1" ).arg( micro1 ) );			// 3
		eleString.append( QString( "X2%1" ).arg( micro1 ) );			// 4
		eleString.append( QString( "Y2%1" ).arg( micro1 ) );			// 5
		eleString.append( QString( "Z2%1" ).arg( micro1 ) );			// 6
		eleString.append( "a11" );										// 7
		eleString.append( "a22" );										// 8
		eleString.append( "a33" );										// 9
		eleString.append( "a12" );										// 10
		eleString.append( "a13" );										// 11
		eleString.append( "a23" );										// 12
		eleString.append( QString( "DimX%1" ).arg( micro1 ) );			// 13
		eleString.append( QString( "DimY%1" ).arg( micro1 ) );			// 14
		eleString.append( QString( "DimZ%1" ).arg( micro1 ) );			// 15
		eleString.append( QString( "phi%1" ).arg( udegree ) );			// 16
		eleString.append( QString( "theta%1" ).arg( udegree ) );		// 17
		eleString.append( QString( "Xm%1" ).arg( micro1 ) );			// 18
		eleString.append( QString( "Ym%1" ).arg( micro1 ) );			// 19
		eleString.append( QString( "Zm%1" ).arg( micro1 ) );			// 20
		//eleString.append( "ShapeFactor" );
		eleString.append( QString( "Volume%1" ).arg( micro3 ) );		// 21
		eleString.append( "Roundness" );								// 22
		eleString.append( QString( "FeretDiam%1" ).arg( micro1 ) );		// 23
		eleString.append( "Flatness" );									// 24
		eleString.append( "VoxDimX" );									// 25
		eleString.append( "VoxDimY" );									// 26
		eleString.append( "VoxDimZ" );									// 27
		eleString.append( "MajorLength" );								// 28
		eleString.append( "MinorLength" );								// 29
	}
	eleString.append( "Class_ID" );	// 25 / 19
	return eleString;
}

void dlg_FeatureScout::SaveBlobMovie()
{
	QString movie_file_types = GetAvailableMovieFormats();

	// If VTK was built without video support,
	// display error message and quit.
	if ( movie_file_types.isEmpty() )
	{
		QMessageBox::information( this, "Movie Export", "Sorry, but movie export support is disabled." );
		return;
	}

	QString mode;
	int imode = 0;
	{
		QStringList modes = ( QStringList() << tr( "No rotation" ) << tr( "Rotate Z" ) << tr( "Rotate X" ) << tr( "Rotate Y" ) );
		QStringList inList = ( QStringList() << tr( "+Rotation mode" ) );
		QList<QVariant> inPara = ( QList<QVariant>() << modes );

		dlg_commoninput dlg( this, "Save movie options", inList, inPara, NULL );
		if ( dlg.exec() == QDialog::Accepted )
		{
			mode = dlg.getComboBoxValue(0);
			imode = dlg.getComboBoxIndex(0);
		}
	}

	QStringList inList = ( QStringList()//TODO: fails if the same string for several widgets
						   << tr( "*Number of frames:" )
						   << tr( "^Range from:" ) << tr( "^Range to:" )
						   << tr( "$Blob body:" )
						   << tr( "^Blob opacity [0,1]:" ) << tr( "^Blob opacity to:" )
						   << tr( "$Silhouettes:" )
						   << tr( "^Silhouettes opacity [0,1]:" ) << tr( "^Silhouettes opacity to:" )
						   << tr( "$3D labels:" )
						   << tr( "$Smart overlapping:" )
						   << tr( "^Separation distance (if smart overlapping):" ) << tr( "^ Separation distance to:" )
						   << tr( "$Smooth after smart overlapping:" )
						   << tr( "$Gaussian blurring of the blob:" )
						   << tr( "*Gaussian blur variance:" ) << tr( "*Gaussian blur variance to:" )
						   << tr( "*Dimension X" ) << tr( "*Dimension X to:" )
						   << tr( "*Dimension Y" ) << tr( "*Dimension Y to:" )
						   << tr( "*Dimension Z" ) << tr( "*Dimension Z to:" )
						   );
	QList<QVariant> inPara;
	inPara
		<< tr( "%1" ).arg( 24 )
		<< tr( "%1" ).arg( blobManager->GetRange() ) << tr( "%1" ).arg( blobManager->GetRange() )
		<< tr( "%1" ).arg( blobManager->GetShowBlob() )
		<< tr( "%1" ).arg( blobManager->GetBlobOpacity() ) << tr( "%1" ).arg( blobManager->GetBlobOpacity() )
		<< tr( "%1" ).arg( blobManager->GetSilhouettes() )
		<< tr( "%1" ).arg( blobManager->GetSilhouetteOpacity() ) << tr( "%1" ).arg( blobManager->GetSilhouetteOpacity() )
		<< tr( "%1" ).arg( blobManager->GetLabeling() )
		<< tr( "%1" ).arg( blobManager->OverlappingIsEnabled() )
		<< tr( "%1" ).arg( blobManager->GetOverlapThreshold() ) << tr( "%1" ).arg( blobManager->GetOverlapThreshold() )
		<< tr( "%1" ).arg( blobManager->GetSmoothing() )
		<< tr( "%1" ).arg( blobManager->GetGaussianBlur() )
		<< tr( "%1" ).arg( blobManager->GetGaussianBlurVariance() ) << tr( "%1" ).arg( blobManager->GetGaussianBlurVariance() )
		<< tr( "%1" ).arg( blobManager->GetDimensions()[0] ) << tr( "%1" ).arg( blobManager->GetDimensions()[0] )
		<< tr( "%1" ).arg( blobManager->GetDimensions()[1] ) << tr( "%1" ).arg( blobManager->GetDimensions()[1] )
		<< tr( "%1" ).arg( blobManager->GetDimensions()[2] ) << tr( "%1" ).arg( blobManager->GetDimensions()[2] );
	dlg_commoninput dlg( this, "Blob rendering preferences", inList, inPara, NULL );

	if ( dlg.exec() == QDialog::Accepted )
	{
		int i = 0;
		double range[2];
		double blobOpacity[2];
		double silhouetteOpacity[2];
		double overlapThreshold[2];
		double gaussianBlurVariance[2];
		int dimX[2]; int dimY[2]; int dimZ[2];

		size_t numFrames = dlg.getIntValue(i++);
		for ( int ind = 0; ind < 2; ++ind )
			range[ind] = dlg.getDblValue(i++);
		blobManager->SetShowBlob( dlg.getCheckValue(i++) != 0 );
		for ( int ind = 0; ind < 2; ++ind )
			blobOpacity[ind] = dlg.getDblValue(i++);
		blobManager->SetSilhouettes( dlg.getCheckValue(i++) != 0 );
		for ( int ind = 0; ind < 2; ++ind )
			silhouetteOpacity[ind] = dlg.getDblValue(i++);
		blobManager->SetLabeling( dlg.getCheckValue(i++) != 0 );
		blobManager->SetOverlappingEnabled( dlg.getCheckValue(i++) != 0 );
		for ( int ind = 0; ind < 2; ++ind )
			overlapThreshold[ind] = dlg.getDblValue(i++);
		blobManager->SetSmoothing( dlg.getCheckValue(i++) );
		blobManager->SetGaussianBlur( dlg.getCheckValue(i++) );
		for ( int ind = 0; ind < 2; ++ind )
			gaussianBlurVariance[ind] = dlg.getIntValue(i++);

		for ( int ind = 0; ind < 2; ++ind )	dimX[ind] = dlg.getIntValue(i++);
		for ( int ind = 0; ind < 2; ++ind )	dimY[ind] = dlg.getIntValue(i++);
		for ( int ind = 0; ind < 2; ++ind )	dimZ[ind] = dlg.getIntValue(i++);

		QFileInfo fileInfo = static_cast<MdiChild*>( activeChild )->getFileInfo();

		blobManager->SaveMovie( activeChild,
								raycaster,
								raycaster->GetRenderer()->GetActiveCamera(),
								raycaster->GetInteractor(),
								raycaster->GetRenderWindow(),
								numFrames,
								range,
								blobOpacity,
								silhouetteOpacity,
								overlapThreshold,
								gaussianBlurVariance,
								dimX, dimY, dimZ,
								QFileDialog::getSaveFileName(
								this,
								tr( "Export movie %1" ).arg( mode ),
								fileInfo.absolutePath() + "/" + ( ( mode.isEmpty() ) ? fileInfo.baseName() : fileInfo.baseName() + "_" + mode ), movie_file_types ),
								imode
								);
	}
	else
		return;
}

bool dlg_FeatureScout::initParallelCoordinates( iAObjectAnalysisType fid )
{
	MdiChild * mdiChild = static_cast<MdiChild*>( activeChild );
	if ( !mdiChild )
		return false;

	iovPC = new dlg_IOVPC( this );
	iovPP = new dlg_IOVPP( this );
	mdiChild->addDockWidget( Qt::BottomDockWidgetArea, this );
	mdiChild->addDockWidget( Qt::BottomDockWidgetArea, iovPC );
	mdiChild->addDockWidget( Qt::BottomDockWidgetArea, iovPP );
	iovPP->colorMapSelection->hide();
	mdiChild->HideHistogram();
	mdiChild->getImagePropertyDlg()->hide();
	mdiChild->logs->hide();
	mdiChild->sYZ->hide();
	mdiChild->sXZ->hide();
	mdiChild->sXY->hide();
	mdiChild->GetModalitiesDlg()->hide();
	if ( this->filterID == INDIVIDUAL_PORE_VISUALIZATION )
		iovPP->hide();

	connect( iovPP->comboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( RenderingOrientation() ) );
	return true;
}

bool dlg_FeatureScout::changeFeatureScout_Options( int idx )
{
	MdiChild * mdiChild = static_cast<MdiChild*>( activeChild );
	if ( !mdiChild )
		return false;

	if ( idx == 6 && !iovSPM )
	{
		iovSPM = new iADockWidgetWrapper("Scatter Plot Matrix", "FeatureScoutSPM");
		mdiChild->addDockWidget( Qt::RightDockWidgetArea, iovSPM );
		iovSPM->show();

		if ( iovDV && !iovMO || ( iovDV && iovMO ) )
		{
			mdiChild->tabifyDockWidget( iovDV, iovSPM);
			iovSPM->show();
			iovSPM->raise();
		}
		else if ( !iovDV && iovMO )
		{
			mdiChild->tabifyDockWidget( iovMO, iovSPM);
			iovSPM->show();
			iovSPM->raise();
		}
	}
	else if ( idx == 6 && iovSPM )
	{
		QMessageBox msgBox;
		msgBox.setText( "Scatterplot Matrix already created." );
		msgBox.setWindowTitle( "FeatureScout" );
		msgBox.exec();
		return false;
	}

	pcChangeOptions( idx );
	return true;
}
