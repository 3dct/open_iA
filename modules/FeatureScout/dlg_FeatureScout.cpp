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
#include "dlg_FeatureScout.h"

#include "dlg_blobVisualization.h"
#include "dlg_editPCClass.h"
#include "iA3DObjectVis.h"
#include "iABlobCluster.h"
#include "iABlobManager.h"
#include "iACsvIO.h"
#include "iAFeatureScoutSPLOM.h"
#include "iAFeatureScoutObjectType.h"
#include "iAMeanObjectTFView.h"

#include "charts/iADiagramFctWidget.h"
#include "dlg_commoninput.h"
#include "dlg_imageproperty.h"
#include "dlg_modalities.h"
#include "iAConsole.h"
#include "iADockWidgetWrapper.h"
#include "iAmat4.h"
#include "iAModalityTransfer.h"
#include "iAMovieHelper.h"
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

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include <QVTKOpenGLWidget.h>
#else
#include <QVTKWidget.h>
#include <vtkRenderWindow.h>
#endif
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAnnotationLink.h>
#include <vtkAxis.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
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
#include <vtkImageCast.h>
#include <vtkIntArray.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLookupTable.h>
#include <vtkMarchingCubes.h>
#include <vtkMath.h>
#include <vtkMathUtilities.h>
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
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarsToColors.h>
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
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
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

namespace
{
	QColor StandardSPLOMDotColor(128, 128, 128, 255);

	enum RenderMode
	{
		rmSingleClass,
		rmMultiClass,
		rmOrientation,
		rmLengthDistribution,
		rmMeanObject
	};
}

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

dlg_FeatureScout::dlg_FeatureScout( MdiChild *parent, iAFeatureScoutObjectType fid, QString const & fileName, vtkRenderer* blobRen,
	vtkSmartPointer<vtkTable> csvtbl, int vis, QSharedPointer<QMap<uint, uint> > columnMapping)
	: QDockWidget( parent ),
	csvTable( csvtbl ),
	raycaster( parent->getRenderer() ),
	elementTableModel(nullptr),
	classTreeModel(new QStandardItemModel()),
	iovSPM(nullptr),
	iovPP(nullptr),
	iovPC(nullptr),
	iovDV(nullptr),
	iovMO(nullptr),
	m_splom(new iAFeatureScoutSPLOM()),
	m_sourcePath( parent->getFilePath() ),
	m_columnMapping(columnMapping),
	m_renderMode(rmSingleClass)
{
	setupUi( this );
	visualization = vis;
	m_pcLineWidth = 0.1;
	m_pcDefaultTextSize = 15;		//Default font size - 17 pt
	m_pcMinTicksCount = 2; //Minimum tick count - 5 ticks to be shown 
	m_pcDefaultTickCount = 10; //default tick number
	this->elementsCount = csvTable->GetNumberOfColumns();
	this->objectsCount = csvTable->GetNumberOfRows();
	this->activeChild = parent;
	this->filterID = fid;
	this->draw3DPolarPlot = false;
	this->setupPolarPlotResolution( 3.0 );
	blobManager = new iABlobManager();
	blobManager->SetRenderers( blobRen, this->raycaster->GetLabelRenderer() );
	double bounds[6];
	if (visualization == iACsvConfig::UseVolume)
	{
		raycaster->GetImageDataBounds(bounds);
		blobManager->SetBounds(bounds);
		blobManager->SetProtrusion(1.5);
		int dimens[3] = { 50, 50, 50 };
		blobManager->SetDimensions(dimens);
	}
	lut = vtkSmartPointer<vtkLookupTable>::New();
	chartTable = vtkSmartPointer<vtkTable>::New();
	chartTable->DeepCopy( csvTable );
	tableList.push_back( chartTable );

	initFeatureScoutUI();
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	pcWidget = new QVTKOpenGLWidget();
	auto pcWidgetRenWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	pcWidget->SetRenderWindow(pcWidgetRenWin);

	polarPlot = new QVTKOpenGLWidget();
	auto polarPlotRenWin = vtkGenericOpenGLRenderWindow::New();
	polarPlot->SetRenderWindow(polarPlotRenWin);
#else
	pcWidget = new QVTKWidget();
	polarPlot = new QVTKWidget();
#endif
	iovPC->setWidget(pcWidget);
	iovPP->verticalLayout->addWidget(polarPlot);
	this->orientationColorMapSelection = iovPP->colorMapSelection;
	this->orientColormap = iovPP->comboBox;

	// Initialize the models for QtViews
	initColumnVisibility();
	setupViews();
	setupModel();
	setupConnections();
	m_3dvis = create3DObjectVis(vis, parent, csvtbl, m_columnMapping, m_colorList.at(0));
	if (vis != iACsvConfig::Lines)
	{
		parent->displayResult(QString("FeatureScout - %1 (%2)").arg(QFileInfo(fileName).fileName())
			.arg(MapObjectTypeToString(filterID)), nullptr, nullptr);
	}
	m_3dvis->show();
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
	delete elementTableModel;
	delete classTreeModel;
}

std::vector<size_t> dlg_FeatureScout::getPCSelection()
{
	std::vector<size_t> selectedIndices;
	vtkSmartPointer<vtkIdTypeArray> pcSelection = pcChart->GetPlot(0)->GetSelection();
#if (VTK_MAJOR_VERSION > 7 || (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION > 0))
	int countSelection = pcSelection->GetNumberOfValues();
#else
	int countSelection = DataSelection->GetNumberOfTuples();
#endif
	for (int idx = 0; idx < countSelection; idx++)
	{
		size_t objID = pcSelection->GetVariantValue(idx).ToUnsignedLongLong();
		selectedIndices.push_back(objID);
	}
	return selectedIndices;
}

void dlg_FeatureScout::pcViewMouseButtonCallBack( vtkObject * , unsigned long, void *, void *, vtkCommand * )
{
	auto classSelectionIndices = getPCSelection();
	m_splom->setFilteredSelection(classSelectionIndices);
	// map from incides inside the class to global indices:
	std::vector<size_t> selectionIndices;
	int classID = activeClassItem->index().row();
	for (size_t filteredSelIdx : classSelectionIndices)
	{
		size_t labelID = tableList[classID]->GetValue(filteredSelIdx, 0).ToUnsignedLongLong();
		selectionIndices.push_back(labelID - 1);
	}
	RenderSelection(selectionIndices);
}

void dlg_FeatureScout::setPCChartData( bool specialRendering )
{
	pcWidget->setEnabled( !specialRendering ); // to disable selection
	if ( specialRendering )
	{   // for the special renderings, we use all data:
		chartTable = csvTable;
	}
	if (pcView->GetScene()->GetNumberOfItems() > 0)
		pcView->GetScene()->RemoveItem(0u);
	this->pcChart = vtkSmartPointer<vtkChartParallelCoordinates>::New();
	this->pcChart->GetPlot( 0 )->SetInputData( chartTable );
	this->pcChart->GetPlot( 0 )->GetPen()->SetOpacity( 90 );
	this->pcChart->GetPlot( 0 )->SetWidth( m_pcLineWidth );
	pcView->GetScene()->AddItem( pcChart );
	pcConnections->Connect(pcChart,
		vtkCommand::SelectionChangedEvent,
		this,
		SLOT(pcViewMouseButtonCallBack(vtkObject*, unsigned long, void*, void*, vtkCommand*)));
	updatePCColumnVisibility();
}

void dlg_FeatureScout::updatePCColumnValues( QStandardItem *item )
{
	if ( item->isCheckable() )
	{
		int i = item->index().row();
		columnVisibility[i] = (item->checkState() == Qt::Checked);
		updatePCColumnVisibility();
		m_splom->setParameterVisibility(i, columnVisibility[i]);
	}
}

void dlg_FeatureScout::spParameterVisibilityChanged(size_t paramIndex, bool enabled)
{
	elementTableModel->item(paramIndex, 0)->setCheckState( enabled ? Qt::Checked : Qt::Unchecked );
	// itemChanged signal from elementTableModel takes care about updating PC (see updatePCColumnValues slot)
}

void dlg_FeatureScout::updatePCColumnVisibility()
{
	
	for ( int j = 0; j < elementsCount; j++ )
	{
		pcChart->SetColumnVisibility( csvTable->GetColumnName( j ), columnVisibility[j]);
	}
	setAxisProperties(m_pcDefaultTextSize, m_pcDefaultTickCount);
	pcView->Update(); 
	pcView->ResetCamera();
	pcView->Render();
	
}

void dlg_FeatureScout::initColumnVisibility()
{
	columnVisibility.resize(elementsCount);
	std::fill(columnVisibility.begin(), columnVisibility.end(), false);
	if (filterID == iAFeatureScoutObjectType::Fibers) // Fibers - (a11, a22, a33,) theta, phi, xm, ym, zm, straightlength, diameter(, volume)
		columnVisibility[(*m_columnMapping)[iACsvConfig::Theta]]   = columnVisibility[(*m_columnMapping)[iACsvConfig::Phi]] =
		columnVisibility[(*m_columnMapping)[iACsvConfig::CenterX]] = columnVisibility[(*m_columnMapping)[iACsvConfig::CenterY]] = columnVisibility[(*m_columnMapping)[iACsvConfig::CenterZ]] =
		columnVisibility[(*m_columnMapping)[iACsvConfig::Length]]  = columnVisibility[(*m_columnMapping)[iACsvConfig::Diameter]] = true;
	else if (filterID == iAFeatureScoutObjectType::Voids) // Pores - (volume, dimx, dimy, dimz,) posx, posy, posz(, shapefactor)
		columnVisibility[(*m_columnMapping)[iACsvConfig::CenterX]] = columnVisibility[(*m_columnMapping)[iACsvConfig::CenterY]] = columnVisibility[(*m_columnMapping)[iACsvConfig::CenterZ]] = true;
}

void dlg_FeatureScout::setupModel()
{
	// setup header data
	elementTableModel->setHeaderData( 0, Qt::Horizontal, tr( "Element" ) );
	elementTableModel->setHeaderData( 1, Qt::Horizontal, tr( "Minimal" ) );
	elementTableModel->setHeaderData( 2, Qt::Horizontal, tr( "Maximal" ) );
	elementTableModel->setHeaderData( 3, Qt::Horizontal, tr( "Average" ) );

	// initialize checkboxes for the first column
	for ( int i = 0; i < elementsCount-1; i++ )
	{
		Qt::CheckState checkState = (columnVisibility[i]) ? Qt::Checked : Qt::Unchecked;
		elementTableModel->setData(elementTableModel->index(i, 0, QModelIndex()), checkState, Qt::CheckStateRole);
		elementTableModel->item( i, 0 )->setCheckable( true );
	}

	// initialize element values
	this->calculateElementTable();
	this->initElementTableModel();

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
	elementTableModel = new QStandardItemModel( elementsCount-1, 4, this );
	elementTable = vtkSmartPointer<vtkTable>::New();

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

	// preparing chart and view for Parallel Coordinates
	this->pcView = vtkContextView::New();
	pcView->SetRenderWindow( pcWidget->GetRenderWindow() );
	pcView->SetInteractor( pcWidget->GetInteractor() );

	// Creates a popup menu
	QMenu* popup2 = new QMenu( pcWidget );
	popup2->addAction( "Add class" );
	popup2->setStyleSheet( "font-size: 11px; background-color: #9B9B9B; border: 1px solid black;" );
	connect( popup2, SIGNAL( triggered( QAction* ) ), this, SLOT( spPopupSelection( QAction* ) ) );

	pcConnections = vtkSmartPointer<vtkEventQtSlotConnect>::New();
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

	setPCChartData(false);
	this->setupPolarPlotView(chartTable);
}

void dlg_FeatureScout::calculateElementTable()
{
	// free table contents first
	elementTable->Initialize();

	double range[2] = { 0, 1 };
	VTK_CREATE( vtkStringArray, nameArr );  // name of columns
	VTK_CREATE( vtkFloatArray, v1Arr );	// minimum
	VTK_CREATE( vtkFloatArray, v2Arr );	// maximal
	VTK_CREATE( vtkFloatArray, v3Arr );	// average
	nameArr->SetNumberOfValues( elementsCount );
	v1Arr->SetNumberOfValues( elementsCount );
	v2Arr->SetNumberOfValues( elementsCount );
	v3Arr->SetNumberOfValues( elementsCount );

	// convert IDs in String to Int
	nameArr->SetValue( 0, csvTable->GetColumnName( 0 ) );
	v1Arr->SetValue( 0, chartTable->GetColumn( 0 )->GetVariantValue( 0 ).ToFloat() );
	v2Arr->SetValue( 0, chartTable->GetColumn( 0 )->GetVariantValue( chartTable->GetNumberOfRows() - 1 ).ToFloat() );
	v3Arr->SetValue( 0, 0 );

	for ( int i = 1; i < elementsCount; i++ )
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

		for ( int i = 0; i < elementsCount-1; i++ ) // number of rows
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
					if ( i == 0 || i == elementsCount - 1 )
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

		for ( int i = 0; i < elementsCount-1; i++ )
		{
			vtkVariant v = chartTable->GetValue( idx, i );
			QString str = QString::number( v.ToDouble(), 'f', 2 );

			if ( i == 0 || i == elementsCount - 1 )
				str = QString::number( v.ToInt() );

			elementTableModel->setData( elementTableModel->index( i, 1, QModelIndex() ), str );
		}
	}
}

void dlg_FeatureScout::initClassTreeModel()
{
	QStandardItem *rootItem = classTreeModel->invisibleRootItem();
	QList<QStandardItem *> stammItem = prepareRow( "Unclassified", QString( "%1" ).arg( objectsCount ), "100" );
	stammItem.first()->setData( QColor( "darkGray" ), Qt::DecorationRole );
	m_colorList.push_back( QColor( "darkGray" ) );

	rootItem->appendRow( stammItem );
	for ( int i = 0; i < objectsCount; ++i )
	{
		vtkVariant v = chartTable->GetColumn( 0 )->GetVariantValue( i );
		QStandardItem *item = new QStandardItem( QString::fromUtf8( v.ToUnicodeString().utf8_str() ).trimmed() );
		stammItem.first()->appendRow( item );
	}
	this->activeClassItem = stammItem.first();
}

// BEGIN DEBUG FUNCTIONS

void dlg_FeatureScout::PrintVTKTable(const vtkSmartPointer<vtkTable> anyTable, const bool useTabSeparator, const QString &outputPath, const QString* fileName) const
{
	std::string separator = (useTabSeparator) ? "\t" : ",";
	ofstream debugfile;
	std::string OutfileName = "";
	if (fileName)
		OutfileName = fileName->toStdString(); 
	else
		OutfileName = "debugFile";

	if (!QDir(outputPath).exists() || !anyTable)
		return;

	debugfile.open(outputPath.toStdString() + OutfileName + ".csv");
	if (!debugfile.is_open())
		return;

	vtkVariant spCol, spRow, spCN, spVal;
	spCol = anyTable->GetNumberOfColumns();
	spRow = anyTable->GetNumberOfRows();

	for (int i = 0; i < spCol.ToInt(); i++)
	{
		spCN = anyTable->GetColumnName(i);
		debugfile << spCN.ToString() << separator;
	}
	debugfile << "\n";	
	for (int row = 0; row < spRow.ToInt(); row++)
	{
		for (int col = 0; col < spCol.ToInt(); col++)
		{
			spVal = anyTable->GetValue(row, col);
			debugfile << spVal.ToString() << separator; //TODO cast debug to double
		}
		debugfile << "\n";
	}
	debugfile.close();
}

void dlg_FeatureScout::PrintChartTable(const QString &outputPath)
{
	QString fileName = "chartTable"; 
	PrintVTKTable(this->chartTable, true, outputPath, &fileName);
}

void dlg_FeatureScout::PrintCSVTable(const QString &outputPath)
{
	QString fileName = "csvTable";
	PrintVTKTable(this->csvTable, true, outputPath, &fileName);
}

void dlg_FeatureScout::PrintTableList(const QList<vtkSmartPointer<vtkTable>> &OutTableList,  QString &outputPath) const  {
	QString FileName ="TableClass";
	QString fID = "";
	QString outPutFile = ""; 
	
	if (OutTableList.count() > 1)
	{	
		for (int i = 0; i < tableList.count(); i++)
		{
			fID = QString(i);
			vtkSmartPointer<vtkTable> OutPutTable = OutTableList[i];
			outPutFile = FileName + "_" + fID;
			this->PrintVTKTable(OutPutTable, true, outputPath, &outPutFile);
			OutPutTable = nullptr; 
		}
	}
}

// END DEBUG FUNCTIONS

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

	connect( this->elementTableModel, SIGNAL( itemChanged( QStandardItem * ) ), this, SLOT( updatePCColumnValues( QStandardItem * ) ) );
	connect( this->classTreeView, SIGNAL( clicked( QModelIndex ) ), this, SLOT( classClicked( QModelIndex ) ) );
	connect( this->classTreeView, SIGNAL( activated( QModelIndex ) ), this, SLOT( classClicked( QModelIndex ) ) );
	connect( this->classTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( classDoubleClicked( QModelIndex ) ) );

	connect( m_splom.data(), &iAFeatureScoutSPLOM::selectionModified, this, &dlg_FeatureScout::spSelInformsPCChart );
	connect( m_splom.data(), &iAFeatureScoutSPLOM::addClass, this, &dlg_FeatureScout::ClassAddButton );
	connect( m_splom.data(), &iAFeatureScoutSPLOM::parameterVisibilityChanged, this, &dlg_FeatureScout::spParameterVisibilityChanged);
}

void dlg_FeatureScout::MultiClassRendering()
{
	m_renderMode = rmMultiClass;
	if ( m_scalarWidgetFLD )
	{   // Turns off FLD scalar bar, updates polar plot view
		m_scalarWidgetFLD->Off();
		this->updatePolarPlotColorScalar(chartTable);
	}
	QStandardItem *rootItem = this->classTreeModel->invisibleRootItem();
	int classCount = rootItem->rowCount();

	if (classCount == 1)
		return;

	double alpha = this->calculateOpacity(rootItem);
	m_splom->multiClassRendering( m_colorList );
	m_splom->enableSelection(false);

	// update lookup table in PC View
	this->updateLookupTable(alpha);
	this->setPCChartData(true);
	static_cast<vtkPlotParallelCoordinates *>(pcChart->GetPlot(0))->SetScalarVisibility(1);
	static_cast<vtkPlotParallelCoordinates *>(pcChart->GetPlot(0))->SetLookupTable(lut);
	static_cast<vtkPlotParallelCoordinates *>(pcChart->GetPlot(0))->SelectColorArray(iACsvIO::ColNameClassID);
	this->pcChart->SetSize(pcChart->GetSize());
	this->pcChart->GetPlot(0)->SetOpacity(0.8);
	pcView->Render();

	m_3dvis->multiClassRendering( m_colorList, rootItem, alpha );
}

void dlg_FeatureScout::SingleRendering( int labelID )
{
	int cID = activeClassItem->index().row();
	QColor classColor = m_colorList.at(cID);
	m_3dvis->renderSingle( labelID, cID, classColor, activeClassItem );
}

void dlg_FeatureScout::RenderSelection( std::vector<size_t> const & selInds )
{
	if ( m_scalarWidgetFLD )
	{   // Turns off FLD scalar bar, updates polar plot view
		m_scalarWidgetFLD->Off();
		this->updatePolarPlotColorScalar( chartTable );
	}
	this->orientationColorMapSelection->hide();
	this->orientColormap->hide();

	if (activeClassItem->rowCount() <= 0)
		return;

	auto sortedSelInds = selInds;
	std::sort(sortedSelInds.begin(), sortedSelInds.end());

	int selectedClassID = activeClassItem->index().row();
	QColor classColor = m_colorList.at(selectedClassID);
	m_3dvis->renderSelection( sortedSelInds, selectedClassID, classColor, activeClassItem );
}

void dlg_FeatureScout::RenderMeanObject()
{
	if (visualization != iACsvConfig::UseVolume)
		return;
	m_renderMode = rmMeanObject;
	int classCount = classTreeModel->invisibleRootItem()->rowCount();
	if ( classCount < 2 )	// unclassified class only
	{
		QMessageBox::warning(this, "FeatureScout", "No defined class (except the 'unclassified' class)." );
		return;
	}
	activeChild->initProgressBar();

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
	if ( activeChild->getImagePointer()->GetScalarType() != 8 )	// long type
	{
		vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
		cast->SetInputData( activeChild->getImagePointer() );
		cast->SetOutputScalarTypeToLong();
		cast->Update();
		vtkToItkConverter->SetInput( cast->GetOutput() );
	}
	else
		vtkToItkConverter->SetInput( static_cast<MdiChild*>( activeChild )->getImagePointer() );
	vtkToItkConverter->Update();

	IType::Pointer itkImageData = vtkToItkConverter->GetOutput();
	double spacing[3];
	spacing[0] = activeChild->getImagePointer()->GetSpacing()[0];
	spacing[1] = activeChild->getImagePointer()->GetSpacing()[1];
	spacing[2] = activeChild->getImagePointer()->GetSpacing()[2];
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
			activeChild->updateProgressBar( percentage );
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
		moHistName.append( QString(" %1 Mean Object").arg(MapObjectTypeToString(filterID)));
		iAModalityTransfer* moHistogram = new iAModalityTransfer( m_MOData.moImageDataList[currClass - 1]->GetScalarRange() );
		m_MOData.moHistogramList.append( moHistogram );

		// Create MObject default Transfer Tunctions
		if ( filterID == iAFeatureScoutObjectType::Fibers ) // Fibers
		{
			m_MOData.moHistogramList[currClass-1]->getColorFunction()->AddRGBPoint( 0.0, 0.0, 0.0, 0.0 );
			m_MOData.moHistogramList[currClass-1]->getColorFunction()->AddRGBPoint( 0.01, 1.0, 1.0, 0.0 );
			m_MOData.moHistogramList[currClass-1]->getColorFunction()->AddRGBPoint( 0.095, 1.0, 1.0, 0.0 );
			m_MOData.moHistogramList[currClass-1]->getColorFunction()->AddRGBPoint( 0.1, 0.0, 0.0, 1.0 );
			m_MOData.moHistogramList[currClass-1]->getColorFunction()->AddRGBPoint( 1.00, 0.0, 0.0, 1.0 );
			m_MOData.moHistogramList[currClass-1]->getOpacityFunction()->AddPoint( 0.0, 0.0 );
			m_MOData.moHistogramList[currClass-1]->getOpacityFunction()->AddPoint( 0.01, 0.01 );
			m_MOData.moHistogramList[currClass-1]->getOpacityFunction()->AddPoint( 0.095, 0.01 );
			m_MOData.moHistogramList[currClass-1]->getOpacityFunction()->AddPoint( 0.1, 0.05 );
			m_MOData.moHistogramList[currClass-1]->getOpacityFunction()->AddPoint( 1.00, 0.18 );
		}
		else // Voids
		{
			m_MOData.moHistogramList[currClass - 1]->getColorFunction()->AddRGBPoint( 0.0, 0.0, 0.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->getColorFunction()->AddRGBPoint( 0.0001, 0.0, 0.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->getColorFunction()->AddRGBPoint( 0.001, 1.0, 1.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->getColorFunction()->AddRGBPoint( 0.18, 1.0, 1.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->getColorFunction()->AddRGBPoint( 0.2, 0.0, 0.0, 1.0 );
			m_MOData.moHistogramList[currClass - 1]->getColorFunction()->AddRGBPoint( 1.0, 0.0, 0.0, 1.0 );
			m_MOData.moHistogramList[currClass - 1]->getOpacityFunction()->AddPoint( 0.0, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->getOpacityFunction()->AddPoint( 0.0001, 0.0 );
			m_MOData.moHistogramList[currClass - 1]->getOpacityFunction()->AddPoint( 0.001, 0.005 );
			m_MOData.moHistogramList[currClass - 1]->getOpacityFunction()->AddPoint( 0.18, 0.005 );
			m_MOData.moHistogramList[currClass - 1]->getOpacityFunction()->AddPoint( 0.2, 0.08 );
			m_MOData.moHistogramList[currClass - 1]->getOpacityFunction()->AddPoint( 1.0, 0.5 );
		}

		// Create the property and attach the transfer functions
		vtkSmartPointer<vtkVolumeProperty> vProperty = vtkSmartPointer<vtkVolumeProperty>::New();
		m_MOData.moVolumePropertyList.append( vProperty );
		vProperty->SetColor( m_MOData.moHistogramList[currClass - 1]->getColorFunction() );
		vProperty->SetScalarOpacity( m_MOData.moHistogramList[currClass - 1]->getOpacityFunction() );
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
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
		m_meanObjectRenderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
		meanObjectWidget = new QVTKOpenGLWidget();
#else
		m_meanObjectRenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
		meanObjectWidget = new QVTKWidget();
#endif
		iovMO->verticalLayout->addWidget( meanObjectWidget );
		meanObjectWidget->SetRenderWindow( m_meanObjectRenderWindow );
		vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
		renderWindowInteractor->SetRenderWindow( m_meanObjectRenderWindow );
		vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
		renderWindowInteractor->SetInteractorStyle( style );

		iovMO->setWindowTitle( QString( "%1 Mean Object View" ).arg(MapObjectTypeToString(filterID)) );

		activeChild->addDockWidget( Qt::RightDockWidgetArea, iovMO );
		iovMO->show();
	}

	// Update MOClass comboBox
	iovMO->cb_Classes->clear();
	for ( int i = 1; i < classCount; ++i )
		iovMO->cb_Classes->addItem( classTreeModel->invisibleRootItem()->child( i, 0 )->text() );

	if ( iovSPM )
	{
		activeChild->tabifyDockWidget(iovSPM, iovMO );
		iovMO->show();
		iovMO->raise();
	}
	else if ( iovDV )
	{
		activeChild->tabifyDockWidget( iovDV, iovMO );
		iovMO->show();
		iovMO->raise();
	}

	// Remove old renderers
	meanObjectWidget->GetRenderWindow()->GetRenderers()->RemoveAllItems();

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
		m_meanObjectRenderWindow->AddRenderer( m_MOData.moRendererList[i] );
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
			cornerAnnotation->GetTextProperty()->SetColor( m_colorList.at( i + 1 ).redF(), m_colorList.at( i + 1 ).greenF(), m_colorList.at( i + 1 ).blueF() );
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
		m_meanObjectRenderWindow->Render();
	}
}

void dlg_FeatureScout::modifyMeanObjectTF()
{
	m_motfView = new iAMeanObjectTFView( this );
	m_motfView->setWindowTitle( QString("%1 %2 Mean Object Transfer Function")
		.arg(iovMO->cb_Classes->itemText(iovMO->cb_Classes->currentIndex()))
		.arg(MapObjectTypeToString(filterID)) );
	iADiagramFctWidget* histogram = activeChild->getHistogram();
	connect( histogram, SIGNAL( updateViews() ), this, SLOT( updateMOView() ) );
	m_motfView->horizontalLayout->addWidget( histogram );
	histogram->show();
	m_motfView->show();
}

void dlg_FeatureScout::updateMOView()
{
	meanObjectWidget->GetRenderWindow()->Render();
}

void dlg_FeatureScout::browseFolderDialog()
{
	QString filename = QFileDialog::getSaveFileName( this, tr( "Save STL File" ), m_sourcePath, tr( "CSV Files (*.stl *.STL)" ) );
	if ( filename.isEmpty() )
		return;
	iovMO->le_StlPath->setText( filename );
}

void dlg_FeatureScout::saveStl()
{
	if ( iovMO->le_StlPath->text().isEmpty() )
	{
		QMessageBox::warning(this, "FeatureScout", "No save file destination specified." );
		return;
	}
	activeChild->initProgressBar();

	iAProgress marCubProgress;
	iAProgress stlWriProgress;
	connect( &marCubProgress, SIGNAL( progress( int ) ), this, SLOT( updateMarProgress( int ) ) );
	connect( &stlWriProgress, SIGNAL( progress( int ) ), this, SLOT( updateStlProgress( int ) ) );

	vtkSmartPointer<vtkMarchingCubes> moSurface = vtkSmartPointer<vtkMarchingCubes>::New();
	marCubProgress.Observe(moSurface);
	moSurface->SetInputData( m_MOData.moImageDataList[iovMO->cb_Classes->currentIndex()] );
	moSurface->ComputeNormalsOn();
	moSurface->ComputeGradientsOn();
	moSurface->SetValue( 0, iovMO->dsb_IsoValue->value() );

	vtkSmartPointer<vtkSTLWriter> stlWriter = vtkSmartPointer<vtkSTLWriter>::New();
	stlWriProgress.Observe(stlWriter);
	stlWriter->SetFileName( iovMO->le_StlPath->text().toStdString().c_str() );
	stlWriter->SetInputConnection( moSurface->GetOutputPort() );
	stlWriter->Write();
}

void dlg_FeatureScout::updateMarProgress(int i)
{
	activeChild->updateProgressBar( i / 2  );
}

void dlg_FeatureScout::updateStlProgress( int i )
{
	activeChild->updateProgressBar( 50 + i / 2 );
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

void dlg_FeatureScout::RenderOrientation()
{
	m_renderMode = rmOrientation;
	if ( m_scalarWidgetFLD )
	{   // Turns off FLD scalar bar, updates polar plot view
		m_scalarWidgetFLD->Off();
		this->updatePolarPlotColorScalar( chartTable );
	}
	setPCChartData(true);
	m_splom->enableSelection(false);
	m_splom->setFilter(-1);

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

	m_3dvis->renderOrientationDistribution( oi );

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
	vtkRenderWindow* renW = polarPlot->GetRenderWindow();
	renW->RemoveRenderer(renW->GetRenderers()->GetFirstRenderer());
	renderer->AddActor( actor );
	renW->AddRenderer(renderer);

	// Projection grid and annotations
	this->drawPolarPlotMesh( renderer );
	this->drawAnnotations( renderer );

	activeChild->updateViews();
	orientationColorMapSelection->show();
	this->orientColormap->show();
}

void dlg_FeatureScout::RenderLengthDistribution()
{
	m_renderMode = rmLengthDistribution;
	setPCChartData(true);
	m_splom->enableSelection(false);
	m_splom->setFilter(-1);

	int numberOfBins;
	double range[2] = { 0.0, 0.0 };
	vtkDataArray* length;

	length = vtkDataArray::SafeDownCast(this->csvTable->GetColumn(m_columnMapping->value(iACsvConfig::Length)));
	QString title = QString("%1 Frequency Distribution").arg(csvTable->GetColumnName(m_columnMapping->value(iACsvConfig::Length)));
	iovPP->setWindowTitle(title);
	numberOfBins = (this->filterID == iAFeatureScoutObjectType::Fibers) ? 8 : 3;

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
	if ( this->filterID == iAFeatureScoutObjectType::Fibers )
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

	m_3dvis->renderLengthDistribution( cTFun, extents, halfInc, filterID, range );

	this->orientationColorMapSelection->hide();
	this->orientColormap->hide();
	this->drawScalarBar( cTFun, this->raycaster->GetRenderer(), 1 );

	// plot length distribution
	VTK_CREATE( vtkContextView, view );
	VTK_CREATE( vtkChartXY, chart );
	chart->SetTitle(title.toUtf8().constData());
	chart->GetTitleProperties()->SetFontSize( (filterID == iAFeatureScoutObjectType::Fibers) ? 15 : 12 );
	vtkPlot *plot = chart->AddPlot( vtkChartXY::BAR );
	plot->SetInputData( fldTable, 0, 1 );
	plot->GetXAxis()->SetTitle( "Length in microns" );
	plot->GetYAxis()->SetTitle( "Frequency" );
	view->GetScene()->AddItem( chart );
	view->SetInteractor( polarPlot->GetInteractor() );
	view->SetRenderWindow( polarPlot->GetRenderWindow() );
	polarPlot->update();
}

void dlg_FeatureScout::ClassAddButton()
{
	vtkIdTypeArray* pcSelection = pcChart->GetPlot(0)->GetSelection();
	int CountObject = pcSelection->GetNumberOfTuples();
	if (CountObject <= 0)
	{
		QMessageBox::warning(this, "FeatureScout", "No object was selected!");
		return;
	}
	if (m_renderMode != rmSingleClass)
	{
		QMessageBox::warning(this, "FeatureScout", "Cannot add a class while in a special rendering mode "
			"(Multi-Class, Fiber Length/Orientation Distribution). "
			"Please click on a class first!");
		return;
	}
	// class name and color
	int cid = classTreeModel->invisibleRootItem()->rowCount();
	QString cText = QString( "Class %1" ).arg( cid );
	QColor cColor = getClassColor(cid);

	bool ok;

	// class name and color input when calling AddClassDialog.
	cText = dlg_editPCClass::getClassInfo( 0, "FeatureScout", cText, &cColor, &ok ).section( ',', 0, 0 );
	if (!ok)
		return;
	m_colorList.push_back( cColor );
	// get the root item from class tree
	QStandardItem *rootItem = classTreeModel->invisibleRootItem();
	QStandardItem *item;

	// create a first level child under rootItem as new class
	double percent = 100.0*CountObject / objectsCount;
	QList<QStandardItem *> firstLevelItem = prepareRow( cText, QString( "%1" ).arg( CountObject ), QString::number( percent, 'f', 1 ) );
	firstLevelItem.first()->setData( cColor, Qt::DecorationRole );

	int classID = rootItem->rowCount();
	int objID = 0;
	QList<int> kIdx; // list to regist the selected index of object IDs in activeClassItem

	// add new class
	for ( int i = 0; i < CountObject; i++ )
	{
		// get objID from item->text()
		vtkVariant v = pcSelection->GetVariantValue( i );
		objID = v.ToInt() + 1;	//fibre index starting at 1 not at 0
		objID = this->activeClassItem->child( v.ToInt() )->text().toInt();

		if ( kIdx.contains(v.ToInt()) )
		{
			DEBUG_LOG(QString("Tried to add objID=%1, v=%2 to class which is already contained in other class!").arg(objID).arg(v.ToInt()));
		}
		else
		{
			kIdx.prepend( v.ToInt() );

			// add item to the new class
			QString str = QString( "%1" ).arg( objID );
			item = new QStandardItem( str );
			firstLevelItem.first()->appendRow( item );

			this->csvTable->SetValue( objID - 1, elementsCount - 1, classID ); // update Class_ID column in csvTable
		}
	}

	// a simple check of the selections
	if ( kIdx.isEmpty() )
	{
		QMessageBox::information(this, "FeatureScout", "Selected objects are already classified, please make a new selection." );
		return;
	}

	if ( kIdx.count() != CountObject )
	{
		QMessageBox::warning(this, "FeatureScout", "Selection Error, please make a new selection." );
		return;
	}
	m_splom->classAdded(classID);

	// append the new class to class tree
	rootItem->appendRow( firstLevelItem );

	// remove items from activeClassItem from table button to top, otherwise you would make a wrong delete
	for ( int i = 0; i < CountObject; i++ )
		this->activeClassItem->removeRow( kIdx.value( i ) );

	// update statistics for activeClassItem
	this->updateClassStatistics( this->activeClassItem );
	if ( this->activeClassItem->rowCount() == 0 && this->activeClassItem->index().row() != 0 )
	{
		int cID = this->activeClassItem->index().row();
		rootItem->removeRow( cID );
		m_colorList.removeAt( cID );
	}

	this->setActiveClassItem( firstLevelItem.first(), 1 );
	this->calculateElementTable();
	this->initElementTableModel();
	this->setPCChartData();
	this->classTreeView->collapseAll();
	this->classTreeView->setCurrentIndex( firstLevelItem.first()->index() );

	this->updatePolarPlotColorScalar(chartTable);
	this->SingleRendering();
}

void dlg_FeatureScout::writeWisetex(QXmlStreamWriter *writer)
{
	if (QMessageBox::warning(this, "FeatureScout",
			"This functionality is only available for legacy fiber/pore csv formats at the moment. Are you sure you want to proceed?",
			QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;
	// Write XML tags using WiseTex specification
	//check if it is a class item
	if ( classTreeModel->invisibleRootItem()->hasChildren() )
	{
		if ( filterID == iAFeatureScoutObjectType::Fibers )
		{
			writer->writeStartElement( "FibreClasses" ); //start FibreClasses tag

			for ( int i = 0; i < classTreeModel->invisibleRootItem()->rowCount(); i++ )
			{
				//Gets the fibre class
				QStandardItem* fc = classTreeModel->invisibleRootItem()->child( i, 1 );

				writer->writeStartElement( QString( "FibreClass-%1" ).arg( i ) ); //start FibreClass-n tag
				writer->writeTextElement( "CColor", QString( "0x00" ).append
										  ( QString( m_colorList.at( fc->index().row() ).name() ).remove( 0, 1 ) ) ); //CColor tag
				writer->writeTextElement( "NFibres", QString( fc->text() ) ); //NFibres tag

				writer->writeStartElement( QString( "Fibres" ) ); //Fibres tag

				for ( int j = 0; j < classTreeModel->invisibleRootItem()->child( i )->rowCount(); ++j )
				{
					writer->writeStartElement( QString( "Fibre-%1" ).arg( j + 1 ) ); //Fibre-n tag

					//Gets fibre features from csvTable
					writer->writeTextElement( "X1",    QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::StartX)).ToString() ) );
					writer->writeTextElement( "Y1",    QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::StartY)).ToString() ) );
					writer->writeTextElement( "Z1",    QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::StartZ)).ToString() ) );
					writer->writeTextElement( "X1",    QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::EndX) ).ToString() ) );
					writer->writeTextElement( "Y2",    QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::EndY) ).ToString() ) );
					writer->writeTextElement( "Z2",    QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::EndZ) ).ToString() ) );
					writer->writeTextElement( "Phi",   QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::Phi)).ToString() ) );
					writer->writeTextElement( "Theta", QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::Theta)).ToString() ) );
					writer->writeTextElement( "Dia",   QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::Diameter)).ToString() ) );
					writer->writeTextElement( "sL",    QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::Length)).ToString() ) );
					// TODO: define mapping?
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
		else if ( filterID == iAFeatureScoutObjectType::Voids )
		{
			writer->writeStartElement( "VoidClasses" ); //start FibreClasses tag

			for ( int i = 0; i < classTreeModel->invisibleRootItem()->rowCount(); i++ )
			{
				//Gets the fibre class
				QStandardItem* fc = classTreeModel->invisibleRootItem()->child( i, 1 );

				writer->writeStartElement( QString( "VoidClass-%1" ).arg( i ) ); //start VoidClass-n tag
				writer->writeTextElement( "CColor", QString( "0x00" ).append
										  ( QString( m_colorList.at( fc->index().row() ).name() ).remove( 0, 1 ) ) ); //CColor tag
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
					writer->writeTextElement( "posX", QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::CenterX)).ToString() ) );
					writer->writeTextElement( "posY", QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::CenterY)).ToString() ) );
					writer->writeTextElement( "posZ", QString( tableList[i]->GetValue( j, m_columnMapping->value(iACsvConfig::CenterZ)).ToString() ) );
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

		inList.append( QString( "#HistoMin for %1" ).arg( columnName ) );
		inList.append( QString( "#HistoMax for %1" ).arg( columnName ) );
		inList.append( QString( "#HistoBinNbr for %1" ).arg( columnName ) );

		inPara.append( this->elementTable->GetColumn( 1 )->GetVariantValue( rowList.at( i ) ).ToFloat() );
		inPara.append( this->elementTable->GetColumn( 2 )->GetVariantValue( rowList.at( i ) ).ToFloat() );
		inPara.append( 100 );
	}

	if ( rowList.count() == 0 )
	{
		QMessageBox::information(this, "FeatureScout", "No characteristic specified in the element explorer." );
		return;
	}

	dlg_commoninput dlg( this, "DistributionViewCSVSaveDialog", inList, inPara, NULL );
	if (dlg.exec() != QDialog::Accepted || (dlg.getCheckValue(0) != 2 && dlg.getCheckValue(1) != 2))
		return;

	QString filename;
	if ( dlg.getCheckValue(0) == 2 )
	{
		filename = QFileDialog::getSaveFileName( this, tr( "Save characteristic distributions" ), m_sourcePath, tr( "CSV Files (*.csv *.CSV)" ) );
		if ( filename.isEmpty() )
			return;
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
		if ( !iovDV )
		{
			iovDV = new iADockWidgetWrapper("Distribution View", "FeatureScoutDV");
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
			auto dvqvtkWidget = new QVTKOpenGLWidget();
			dvqvtkWidget->SetRenderWindow(vtkGenericOpenGLRenderWindow::New());
#else
			auto dvqvtkWidget = new QVTKWidget();
#endif
			iovDV->setWidget(dvqvtkWidget);
			m_dvContextView->SetRenderWindow( dvqvtkWidget->GetRenderWindow() );
			m_dvContextView->SetInteractor( dvqvtkWidget->GetInteractor() );
			activeChild->addDockWidget( Qt::RightDockWidgetArea, iovDV );
			iovDV->show();
		}

		if ( iovSPM && !iovMO || ( iovSPM && iovMO ) )
		{
			activeChild->tabifyDockWidget( iovSPM, iovDV );
			iovDV->show();
			iovDV->raise();
		}
		else if ( !iovSPM && iovMO )
		{
			activeChild->tabifyDockWidget( iovMO, iovDV );
			iovDV->show();
			iovDV->raise();
		}
		this->m_dvContextView->GetScene()->AddItem( distributionChartMatrix.GetPointer() );
		this->m_dvContextView->GetRenderWindow()->Render();
	}
}

void dlg_FeatureScout::WisetexSaveButton()
{
	if ( !classTreeModel->invisibleRootItem()->hasChildren() )
	{
		QMessageBox::warning(this, "FeatureScout", "No data available!" );
		return;
	}

	//XML file save path
	QString filename = QFileDialog::getSaveFileName( this, tr( "Save File" ), m_sourcePath, tr( "XML Files (*.xml *.XML)" ) );
	if ( filename.isEmpty() )
		return;

	QFile file( filename );
	if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		QMessageBox::warning(this, "FeatureScout", "Could not open XML file for writing." );
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
		QMessageBox::warning(this, "FeatureScout", "No classes was defined." );
		return;
	}

	QString filename = QFileDialog::getSaveFileName( this, tr( "Save File" ), m_sourcePath, tr( "XML Files (*.xml *.XML)" ) );
	if ( filename.isEmpty() )
		return;

	QFile file( filename );
	if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		QMessageBox::warning(this, "FeatureScout", "Could not open XML file for writing." );
		return;
	}

	QXmlStreamWriter stream( &file );

	stream.setAutoFormatting( true );
	stream.writeStartDocument();
	stream.writeStartElement( IFVTag );
	stream.writeAttribute( VersionAttribute, "1.0" );
	stream.writeAttribute( CountAllAttribute, QString( "%1" ).arg( objectsCount ) );

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
	QString filename = QFileDialog::getOpenFileName( this, tr( "Load xml file" ), m_sourcePath, tr( "XML Files (*.xml *.XML)" ) );
	if ( filename.isEmpty() )
		return;

	QFile file( filename );
	if ( !file.open( QIODevice::ReadOnly ) )
	{
		QMessageBox::warning(this, "FeatureScout", "Class load error: Could not open source xml file." );
		return;
	}

	// checking xml file correctness
	QXmlStreamReader checker( &file );
	checker.readNext();
	checker.readNext();
	if ( checker.name() == IFVTag )
	{
		// if the object number is not correct, stop the load process
		if ( checker.attributes().value( CountAllAttribute ).toString().toInt() != this->objectsCount )
		{
			QMessageBox::warning(this, "FeatureScout", "Class load error: Incorrect xml file for current dataset, please check." );
			checker.clear();
			return;
		}
	}
	else // incompatible xml file
	{
		QMessageBox::warning(this, "FeatureScout", "Class load error: xml file incompatible with FeatureScout, please check." );
		checker.clear();
		return;
	}
	checker.clear();
	file.close();

	chartTable->DeepCopy( csvTable ); // reset charttable
	tableList.clear();
	tableList.push_back( chartTable );
	m_colorList.clear();
	classTreeModel->clear();

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
				QString label = reader.attributes().value(
					(filterID == iAFeatureScoutObjectType::Fibers) ? LabelAttribute : LabelAttributePore ).toString();
				QStandardItem *item = new QStandardItem( label );

				// add objects to firstLevelClassItem
				activeItem->appendRow( item );

				// update Class_ID number in csvTable;
				this->csvTable->SetValue( label.toInt() - 1, this->elementsCount - 1, idxClass - 1 );
				m_splom->changeClass(label.toInt() - 1, idxClass - 1);
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
				m_colorList.append( QColor( color ) );
				rootItem->appendRow( stammItem );
				activeItem = rootItem->child( idxClass );
				idxClass++;
			}
		}
	}
	m_splom->classesChanged();

	//upadate TableList
	if ( rootItem->rowCount() == idxClass )
	{
		for ( int i = 0; i < idxClass; i++ )
			this->recalculateChartTable( rootItem->child( i ) );
		this->setActiveClassItem( rootItem->child( 0 ), 0 );
	}
	else
	{
		QMessageBox::warning(this, "FeatureScout", "Class load error: unclear class load process." );
	}
	reader.clear();
	readFile.close();
	if ( m_splom->isShown() )
	{
		// reinitialize spm
		activeChild->removeDockWidget( iovSPM );
		delete iovSPM;
		iovSPM = nullptr;
		showScatterPlot();
	}
}

void dlg_FeatureScout::ClassDeleteButton()
{
	QStandardItem *rootItem = this->classTreeModel->invisibleRootItem();
	QStandardItem *stammItem = rootItem->child( 0 );
	if ( this->activeClassItem->index().row() == 0 )
	{
		QMessageBox::warning(this, "FeatureScout", "You are trying to delete the unclassified class, please select another class." );
		return;
	}

	// define a list to sort the items in stammItem
	QList<int> list;
	// get Class_ID
	int deleteClassID = this->activeClassItem->index().row();
	int countActive = this->activeClassItem->rowCount();

	// append stamm item values to list
	for ( int i = 0; i < stammItem->rowCount(); i++ )
		list.append( stammItem->child( i )->text().toInt() );
	for ( int j = 0; j < countActive; j++ )
	{
		int labelID = this->activeClassItem->child( j )->text().toInt();
		// update Class_ID column, prepare values for LookupTable
		this->csvTable->SetValue(labelID - 1, elementsCount - 1, 0);
		// append the deleted object IDs to list
		list.append(labelID);
	}
	m_splom->classDeleted(deleteClassID);
	// sort the new stamm list
	qSort( list );
	// give the values from list to stammitem
	stammItem->removeRows( 0, stammItem->rowCount() );
	for ( int k = 0; k < list.size(); k++ )
	{
		QStandardItem *item = new QStandardItem( QString( "%1" ).arg( list.at( k ) ) );
		stammItem->appendRow( item );
	}

	// remove the deleted class from tree view, its entry in tableList and its color
	tableList.removeAt(deleteClassID);
	rootItem->removeRow(deleteClassID);
	m_colorList.removeAt(deleteClassID);

	// Update class ID for all remaining classes elements
	int classCount = rootItem->rowCount();

	//set new class ID, iterate 
	if (classCount > 0)
	{
		for (int classID = deleteClassID; classID < classCount; ++classID)
		{
			QStandardItem *item = rootItem->child(classID, 0);

			//go for each element in the class and reset id
			//element number = number of colums
			for (int j = 0; j < item->rowCount(); j++)
			{
				int labelID = item->child(j, 0)->text().toInt();
				this->csvTable->SetValue(labelID - 1, elementsCount - 1, classID);
			}
			for (int k = 0; k < tableList[classID]->GetNumberOfRows(); ++k)
			{
				tableList[classID]->SetValue(k, elementsCount - 1, classID);
			}
			tableList[classID]->GetColumn(classID)->Modified();
		}
	}

	// update statistics for activeClassItem
	this->updateClassStatistics(stammItem);

	// update tableList and setup activeClassItem
	this->setActiveClassItem( stammItem, 2 );
	QSignalBlocker ctvBlocker(classTreeView);
	classTreeView->setCurrentIndex(classTreeView->model()->index(0, 0));

	// update element view
	this->setPCChartData();
	this->calculateElementTable();
	this->initElementTableModel();
	this->SingleRendering();
	if (m_renderMode != rmSingleClass)
	{
		m_renderMode = rmSingleClass;
		// reset color in SPLOM
		m_splom->setDotColor(StandardSPLOMDotColor);
		m_splom->enableSelection(true);
	}
}

void dlg_FeatureScout::showScatterPlot()
{
	if (iovSPM)
		return;
	iovSPM = new iADockWidgetWrapper("Scatter Plot Matrix", "FeatureScoutSPM");
	activeChild->addDockWidget(Qt::RightDockWidgetArea, iovSPM);
	iovSPM->show();
	if (iovDV && !iovMO || (iovDV && iovMO))
	{
		activeChild->tabifyDockWidget(iovDV, iovSPM);
		iovSPM->show();
		iovSPM->raise();
	}
	else if (!iovDV && iovMO)
	{
		activeChild->tabifyDockWidget(iovMO, iovSPM);
		iovSPM->show();
		iovSPM->raise();
	}
	m_splom->initScatterPlot(iovSPM, csvTable, columnVisibility);
	if (m_renderMode == rmMultiClass)
		m_splom->multiClassRendering(m_colorList);
	else
	{
		m_splom->setDotColor(StandardSPLOMDotColor);
		if (m_renderMode == rmSingleClass)
		{
			auto filteredSelInds = getPCSelection();
			m_splom->setFilteredSelection(filteredSelInds);
			QStandardItem* item = activeClassItem;
			if (item)
			{
				if (!item->hasChildren())
					item = item->parent();
				int classID = item->row();
				m_splom->setFilter(classID);
			}
		}
	}
}

void dlg_FeatureScout::spSelInformsPCChart(std::vector<size_t> const & selInds)
{	// If scatter plot selection changes, Parallel Coordinates gets informed
	RenderSelection(selInds);
	QCoreApplication::processEvents();
	auto sortedSelInds = m_splom->getFilteredSelection();
	int countSelection = sortedSelInds.size();
	vtkSmartPointer<vtkIdTypeArray> vtk_selInd = vtkSmartPointer<vtkIdTypeArray>::New();
	vtk_selInd->Allocate(countSelection);
	vtk_selInd->SetNumberOfValues(countSelection);
	int idx = 0;
	for (auto ind: sortedSelInds)
	{
		vtkVariant var_Idx = ind;
		long long curr_selInd = var_Idx.ToLongLong() /*+1*/;
		vtk_selInd->SetVariantValue(idx, curr_selInd);
		++idx;
	}
	pcChart->GetPlot(0)->SetSelection(vtk_selInd);
	pcView->Render();
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
		QMenu* popupMenu = static_cast<QMenu*>( client_data );
		int* sz = iren->GetSize();  // Get event location
		QPoint pt = QPoint( mouseReleasePos[0], sz[1] - mouseReleasePos[1] );  // flip y
		QPoint global_pt = popupMenu->parentWidget()->mapToGlobal( pt );
		popupMenu->popup( global_pt );
	}
	else
	{
		QMenu* popupMenu = static_cast<QMenu*>( client_data );     // Reset to original SPM popup
		if ( popupMenu->actions().count() > 1 )                    // Check if SPM or PC popups
			popupMenu->actions().at( 1 )->setText( "Suggest Classification" );
	}
}

void dlg_FeatureScout::spPopupSelection( QAction *selection )
{
	if (selection->text() == "Add class") { ClassAddButton(); }
	// TODO SPM
	if ( selection->text() == "Suggest Classification" )
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
			QColor cColor = getClassColor(cid);
			m_colorList.append( cColor );

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
			QMessageBox::warning(this, "FeatureScout", "No object was selected!" );
			// return; (?)
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
		m_colorList.removeAt( cID );
		//update Class_ID and lookupTable??
	}

	this->calculateElementTable();
	this->initElementTableModel();
	this->setPCChartData();
	this->classTreeView->collapseAll();
	this->SingleRendering();
	this->updatePolarPlotColorScalar( chartTable );

	//Updates scatter plot matrix when a class is added.
	// TODO SPM
	//matrix->UpdateColorInfo( classTreeModel, m_colorList );
	//matrix->SetClass2Plot( this->activeClassItem->index().row() );
	//matrix->UpdateLayout();
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
	else	// An 'undefined area' was clicked.
		return;

	this->pcWidget->setEnabled( true );

	if ( item->hasChildren() )
	{
		int classID = index.row();
		bool ok;
		QColor old_cColor = m_colorList.at( classID );
		QString old_cText = item->text();
		QColor new_cColor = old_cColor;
		QString new_cText = old_cText;
		new_cText = dlg_editPCClass::getClassInfo( 0, "Class Explorer", new_cText, &new_cColor, &ok ).section( ',', 0, 0 );

		if ( ok && ( old_cText.compare( new_cText ) != 0 || new_cColor != old_cColor ) )
		{
			m_colorList[index.row()] = new_cColor;
			item->setText( new_cText );
			item->setData( new_cColor, Qt::DecorationRole );
			this->SingleRendering();
			m_splom->clearSelection();
			m_splom->setFilter(classID);
			m_splom->setDotColor(StandardSPLOMDotColor);
		}
	}
}

void dlg_FeatureScout::classClicked( const QModelIndex &index )
{
	if ( m_scalarWidgetFLD )
	{   // Turns off FLD scalar bar, updates polar plot view
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
		QMessageBox::information(this, "FeatureScout", "Unclassified class contains no objects, please make another selection." );
		return;
	}
	//MOD kMeans
	if ( item->rowCount() == 0 && item->parent()->index() == this->classTreeModel->invisibleRootItem()->index() )
	{
		QMessageBox::information(this, "FeatureScout", "This class contains no object, please make another selection or delete the class." );
		return;
	}
	QStandardItem* classItem = item->hasChildren() ? item : item->parent();
	if (classItem != activeClassItem || m_renderMode != rmSingleClass)
	{
		int classID = classItem->index().row();
		m_splom->setFilter(classID);
		setActiveClassItem(classItem);
		calculateElementTable();
		setPCChartData();
		updatePolarPlotColorScalar(chartTable);
		if (item->hasChildren())  // has children => a class was selected
		{
			SingleRendering();
			initElementTableModel();
			m_splom->clearSelection();
		}
	}
	if (m_renderMode != rmSingleClass)  // special rendering was enabled before
	{
		m_renderMode = rmSingleClass;
		// reset color in SPLOM
		m_splom->setDotColor(StandardSPLOMDotColor);
		m_splom->enableSelection(true);
	}
	if (!item->hasChildren()) // has no children => single object selected
	{
		// update ParallelCoordinates view selection
		int sID = item->index().row();

		// Fill selection with IDs
		vtkSmartPointer<vtkIdTypeArray> testArr = vtkSmartPointer<vtkIdTypeArray>::New();
		testArr->SetName( "Label" );
		testArr->InsertNextValue( sID );

		this->pcChart->GetPlot( 0 )->SetSelection( testArr );
		pcView->ResetCamera();
		pcView->Render();

		// update elementTableView
		this->initElementTableModel( sID );
		int labelID = item->text().toInt();
		this->SingleRendering(labelID);
		elementTableView->update();

		// update SPLOM selection
		std::vector<size_t> filteredSelInds;
		filteredSelInds.push_back(sID);
		m_splom->setFilteredSelection(filteredSelInds);
	}
}

double dlg_FeatureScout::calculateOpacity( QStandardItem *item )
{
	// chart opacity dependence of number of objects
	// for multi rendering
	if ( item == this->classTreeModel->invisibleRootItem() )
	{
		if ( objectsCount < 1000 )
			return 1.0;
		if ( objectsCount < 3000 )
			return 0.8;
		if ( objectsCount < 10000 )
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

		QString color = QString( m_colorList.at( item->index().row() ).name() );

		writer->writeAttribute( ColorAttribute, color );
		writer->writeAttribute( CountAttribute, classTreeModel->invisibleRootItem()->child( item->index().row(), 1 )->text() );
		writer->writeAttribute( PercentAttribute, classTreeModel->invisibleRootItem()->child( item->index().row(), 2 )->text() );
		for ( int i = 0; i < item->rowCount(); i++ )
		{
			writer->writeStartElement( ObjectTag );
			for ( int j = 0; j < elementsCount; j++ )
			{
				vtkVariant v = csvTable->GetValue( item->child( i )->text().toInt() - 1, j );
				QString str = QString::fromUtf8( v.ToUnicodeString().utf8_str() ).trimmed();
				vtkVariant v1 = elementTable->GetValue( j, 0 );
				QString str1 = QString::fromUtf8( v1.ToUnicodeString().utf8_str() ).trimmed();
				writer->writeAttribute( str1, str );
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
		this->activeClassItem = item;
		int id = item->index().row();
		chartTable = tableList[id];
	}
	else if ( situ == 1 )	// add class
	{
		// recalculate the old active class
		this->recalculateChartTable( this->activeClassItem );

		// calculate the new class table and set up chartTable
		this->recalculateChartTable( item );

		this->activeClassItem = item;
		int id = item->index().row();
		chartTable = tableList[id];
	}
	else if ( situ == 2 )	// delete class
	{
		// merge the deleted class table to stamm table
		this->recalculateChartTable( item );
		this->activeClassItem = item;
		chartTable = tableList[0];
	}
}

void dlg_FeatureScout::recalculateChartTable( QStandardItem *item )
{
	if ( !item->hasChildren() )
		return;

	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
	vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName( chartTable->GetColumnName( 0 ) );
	table->AddColumn( arr );
	for ( int i = 1; i < elementsCount - 1; i++ )
	{
		vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
		arrX->SetName( chartTable->GetColumnName( i ) );
		table->AddColumn( arrX );
	}
	vtkSmartPointer<vtkIntArray> arrI = vtkSmartPointer<vtkIntArray>::New();
	arrI->SetName( chartTable->GetColumnName( elementsCount - 1 ) );
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
		// add the new active class table to tableList
		tableList.insert( itemID, table );
		// delete the old active class table
		tableList.removeAt( itemID + 1 );
	}
	else
	{
		// add the new table to the end of the tableList
		// maka a copy to chartTable
		tableList.push_back( table );
		chartTable = tableList[item->index().row()];
	}
}

void dlg_FeatureScout::updateLookupTable( double alpha )
{
	int lutNum = m_colorList.size();
	lut->SetNumberOfTableValues( lutNum );
	for ( int i = 0; i < lutNum; i++ )
		lut->SetTableValue( i,
		m_colorList.at( i ).red() / 255.0,
		m_colorList.at( i ).green() / 255.0,
		m_colorList.at( i ).blue() / 255.0,
		m_colorList.at( i ).alpha() / 255.0 );

	lut->SetRange( 0, lutNum - 1 );
	lut->SetAlpha( alpha );
	lut->Build();
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
		//int index = chartTable->GetValue( i, 0 ).ToInt();
		fi.x1 = chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::StartX)).ToDouble();
		fi.y1 = chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::StartY)).ToDouble();
		fi.z1 = chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::StartZ)).ToDouble();
		fi.x2 = chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::EndX)).ToDouble();
		fi.y2 = chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::EndY)).ToDouble();
		fi.z2 = chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::EndZ)).ToDouble();
		fi.diameter = chartTable->GetValue(i, m_columnMapping->value(iACsvConfig::Diameter)).ToDouble();
		objects.append( fi );
	}
	blob->SetObjectType( MapObjectTypeToString(filterID) );
	blob->SetLabelScale( 10.0 * maxDim / 100.0 );
	blob->SetCluster( objects );

	// set color
	QColor color = m_colorList.at( activeClassItem->index().row() );
	blob->GetSurfaceProperty()->SetColor( color.redF(), color.greenF(), color.blueF() );
	blob->SetName( activeClassItem->text() );
	const double count = activeClassItem->rowCount();
	const double percentage = 100.0*count / objectsCount;
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
		else if ( item->parent() ) // item also might have no children because it's an empty class!
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
	QMessageBox::warning(this, "FeatureScout", "Adding an object to the active class is not implemented yet!" );
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
		QMessageBox::information(this, "FeatureScout", "An object in the unclassified class can not be deleted.");
		return;
	}

	if ( item->parent()->rowCount() == 1 )
	{
		this->ClassDeleteButton();
	}
	else
	{
		int oID = item->text().toInt();
		this->csvTable->SetValue( oID - 1, elementsCount - 1, 0 );

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
		double percent = 100.0*item->rowCount() / objectsCount;
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
		fp = t->GetValue( k, m_columnMapping->value(iACsvConfig::Phi)).ToDouble() / PolarPlotPhiResolution;
		ft = t->GetValue( k, m_columnMapping->value(iACsvConfig::Theta)).ToDouble() / PolarPlotThetaResolution;
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
			m_scalarWidgetPP->SetInteractor( polarPlot->GetInteractor() );
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

void dlg_FeatureScout::setupPolarPlotView( vtkTable *it )
{
	if (!m_columnMapping->contains(iACsvConfig::Phi) || !m_columnMapping->contains(iACsvConfig::Theta))
	{
		DEBUG_LOG("It wasn't defined in which columns the angles phi and theta can be found, cannot set up polar plot view");
		return;
	}
	iovPP->setWindowTitle( "Orientation Distribution" );
	double xx, yy, zz, phi;

	// construct delaunay triangulation
	delaunay = vtkDelaunay2D::New();

	// create point sets
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

	// calculate object probability and save it to a table
	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
	int pcMaxC = this->calcOrientationProbability( it, table ); // maximal count of the object orientation

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

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	VTK_CREATE( vtkGenericOpenGLRenderWindow, renW );
#else
	VTK_CREATE( vtkRenderWindow, renW );
#endif
	renW->AddRenderer( renderer );
	renderer->AddActor( actor );

	vtkRenderWindowInteractor *renI = polarPlot->GetInteractor();
	renI->SetRenderWindow( renW );
	polarPlot->SetRenderWindow( renW );

	this->drawPolarPlotMesh( renderer );
	this->drawAnnotations( renderer );
	this->drawScalarBar( cTFun, renderer, 0 );
	polarPlot->GetRenderWindow()->Render();
}

void dlg_FeatureScout::updatePolarPlotColorScalar( vtkTable *it )
{
	if (!m_columnMapping->contains(iACsvConfig::Phi) || !m_columnMapping->contains(iACsvConfig::Theta))
	{
		DEBUG_LOG("It wasn't defined in which columns the angles phi and theta can be found, cannot set up polar plot scalar");
		return;
	}
	iovPP->setWindowTitle( "Orientation Distribution" );
	double xx, yy, zz, phi;

	// calculate object probability and save it to a table
	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

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

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	VTK_CREATE( vtkGenericOpenGLRenderWindow, renW );
#else
	VTK_CREATE( vtkRenderWindow, renW );
#endif
	renW->AddRenderer( renderer );
	renderer->AddActor( actor );

	vtkRenderWindowInteractor *renI = polarPlot->GetInteractor();
	renI->SetRenderWindow( renW );
	polarPlot->SetRenderWindow( renW );

	this->drawPolarPlotMesh( renderer );
	this->drawAnnotations( renderer );
	this->drawScalarBar( cTFun, renderer );
	polarPlot->GetRenderWindow()->Render();
}

void dlg_FeatureScout::setupPolarPlotResolution( float grad )
{
	this->gPhi = vtkMath::Floor( 360.0 / grad );
	this->gThe = vtkMath::Floor( 90.0 / grad );
	this->PolarPlotPhiResolution = 360.0 / gPhi;
	this->PolarPlotThetaResolution = 90.0 / gThe;
	gThe = gThe + 1;
}

bool dlg_FeatureScout::OpenBlobVisDialog()
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
	if ( dlg.exec() != QDialog::Accepted )
		return false;
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
	return true;
}

void dlg_FeatureScout::SaveBlobMovie()
{
	QString movie_file_types = GetAvailableMovieFormats();
	if ( movie_file_types.isEmpty() )
	{
		QMessageBox::information( this, "Movie Export", "Sorry, but movie export support is disabled." );
		return;
	}
	QStringList modes = ( QStringList() << tr( "No rotation" ) << tr( "Rotate Z" ) << tr( "Rotate X" ) << tr( "Rotate Y" ) );
	QStringList inList = ( QStringList() << tr( "+Rotation mode" ) );
	QList<QVariant> inPara = ( QList<QVariant>() << modes );
	dlg_commoninput dlg( this, "Save movie options", inList, inPara, NULL );
	if ( dlg.exec() != QDialog::Accepted )
		return;
	QString mode = dlg.getComboBoxValue(0);
	int imode = dlg.getComboBoxIndex(0);
	inList.clear();
	inList = ( QStringList()
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
	inPara.clear();
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
	dlg_commoninput dlg2( this, "Blob rendering preferences", inList, inPara, NULL );
	if ( dlg2.exec() != QDialog::Accepted )
		return;
	
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

	QFileInfo fileInfo = activeChild->getFileInfo();

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

void dlg_FeatureScout::initFeatureScoutUI()
{
	iovPC = new iADockWidgetWrapper("Parallel Coordinates",  "FeatureScoutPC");
	iovPP = new dlg_IOVPP( this );
	activeChild->addDockWidget( Qt::RightDockWidgetArea, this );
	activeChild->addDockWidget( Qt::RightDockWidgetArea, iovPC );
	activeChild->addDockWidget( Qt::RightDockWidgetArea, iovPP );
	iovPP->colorMapSelection->hide();
	if (this->filterID == iAFeatureScoutObjectType::Voids)
		iovPP->hide();
	connect(iovPP->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(RenderOrientation()));

	if (visualization == iACsvConfig::UseVolume)
		activeChild->getImagePropertyDlg()->hide();
	activeChild->HideHistogram();
	activeChild->logs->hide();
	activeChild->sYZ->hide();
	activeChild->sXZ->hide();
	activeChild->sXY->hide();
	activeChild->GetModalitiesDlg()->hide();
}

void dlg_FeatureScout::changeFeatureScout_Options( int idx )
{
	switch (idx)
	{
	case 0:         // option menu, do nothing
	default:
		break;
	case 1:			// blob Rendering, trigger OpenBlobVisDialog()
		this->OpenBlobVisDialog();
		break;

	case 3: MultiClassRendering();      break;
	case 4: RenderMeanObject();         break;
	case 5: RenderOrientation();        break;
	case 7: RenderLengthDistribution(); break;

	case 6:			// plot scatterplot matrix
		if (iovSPM)
		{
			QMessageBox::information(this, "FeatureScout", "Scatterplot Matrix already created.");
			return;
		}
		showScatterPlot();
		break;
	}
}


void dlg_FeatureScout::setAxisProperties(int fontSize, int tickCount)
{
	int axis_count = pcChart->GetNumberOfAxes();
	for (int i = 0; i < axis_count; i++) {
		vtkAxis *axis = pcChart->GetAxis(i);
		setAxisTickCount(axis, m_pcDefaultTickCount, false);
		setAxisFontSize(axis, m_pcDefaultTextSize, false);
	}
	
	pcChart->Update(); 
}

void dlg_FeatureScout::setAxisFontSize(vtkAxis * axis, int fontSize, bool updatePC) {
	if (axis && (fontSize > 0)){
		axis->GetLabelProperties()->SetFontSize(fontSize);
		axis->GetTitleProperties()->SetFontSize(fontSize);
		axis->RecalculateTickSpacing(); 
		if (updatePC) {
			pcChart->Update(); 
		}

	}
}

void dlg_FeatureScout::setAxisTickCount(vtkAxis *axis, int tickCount, bool updatePC)
{
	if (axis && (tickCount > m_pcMinTicksCount)) {
		axis->SetNumberOfTicks(tickCount);
		axis->RecalculateTickSpacing(); 
		if (updatePC) {
			pcChart->Update(); 
		}
	}
	// TODO else invalid axis; 
}



