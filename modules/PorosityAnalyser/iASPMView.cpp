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
#include "iASPMView.h"

#include "iAQtVTKBindings.h"
#include "PorosityAnalyserHelpers.h"
#include "iASelection.h"
#include "iASPMSettings.h"
#include "iAPerceptuallyUniformLUT.h"
#include "iAPAQSplom.h"

#include <QVTKOpenGLWidget.h>
#include <vtkAnnotationLink.h>
#include <vtkChart.h>
#include <vtkColor.h>
#include <vtkColorTransferFunction.h>
#include <vtkContextMouseEvent.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkDoubleArray.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkFloatArray.h>
#include <vtkIdTypeArray.h>
#include <vtkInteractorObserver.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkPen.h>
#include <vtkPlot.h>
#include <vtkPlotPoints.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarActor.h>
#include <vtkScatterPlotMatrix.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>

#include <QCheckBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QMenu>
#include <QTableWidget>
#include <QVBoxLayout>


const QString defaultColorParam = "Deviat. from Ref.";
const int popupWidthRange[2] = { 80, 300 };

iASPMView::iASPMView( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	: PorosityAnalyzerSPMConnector( parent, f ),
	m_SPMSettings( new iASPMSettings( this, f ) ),
	m_SPLOMSelection( vtkSmartPointer<vtkIdTypeArray>::New() ),
	m_lut( vtkSmartPointer<vtkLookupTable>::New() ),
	m_SBQVTKWidget( new QVTKOpenGLWidget( this ) ),
	m_sbRen( vtkSmartPointer<vtkRenderer>::New() ),
	m_sbActor( vtkSmartPointer<vtkScalarBarActor>::New() ),
	m_colorArrayName( defaultColorParam ),
	m_updateColumnVisibility( true ),
	m_splom( new iAPAQSplom( parent ) )
{
	QHBoxLayout *layoutHB2 = new QHBoxLayout( this );
	layoutHB2->setMargin( 0 );
	layoutHB2->setSpacing( 0 );
	layoutHB2->addWidget( m_splom );
	SPLOMWidget->setLayout( layoutHB2 );

	InitLUT();
	InitScalarBar();

	connect( m_SPMSettings->parametersList, SIGNAL( itemChanged( QListWidgetItem * ) ), this, SLOT( changeColumnVisibility( QListWidgetItem * ) ) );
	connect( m_SPMSettings->colorCodingParameter, SIGNAL( currentIndexChanged( const QString & ) ), this, SLOT( SetParameterToColorcode( const QString & ) ) );
	connect( m_SPMSettings->sbMin, SIGNAL( valueChanged( double ) ), this, SLOT( UpdateLookupTable() ) );
	connect( m_SPMSettings->sbMax, SIGNAL( valueChanged( double ) ), this, SLOT( UpdateLookupTable() ) );
	connect( m_SPMSettings->opacitySlider, SIGNAL( valueChanged( int ) ), this, SLOT( UpdateLookupTable() ) );
	connect( tbSettings, SIGNAL( clicked() ), this, SLOT( showSettings() ) );
	connect( m_splom, SIGNAL( selectionModified( QVector<unsigned int>* ) ), this, SLOT( selectionUpdated( QVector<unsigned int>* ) ) );
	connect( m_splom, SIGNAL( previewSliceChanged( int ) ), this, SIGNAL( previewSliceChanged( int ) ) );
	connect( m_splom, SIGNAL( sliceCountChanged( int ) ), this, SIGNAL( sliceCountChanged( int ) ) );
	connect( m_splom, SIGNAL( maskHovered( const QPixmap *, int ) ), this, SIGNAL( maskHovered( const QPixmap *, int ) ) );
}

void iASPMView::InitScalarBar()
{
	m_sbRen->SetBackground( 1.0, 1.0, 1.0 );
	m_sbRen->AddActor( m_sbActor );
	m_sbActor->SetAnnotationTextScaling( 0 );
	m_sbActor->GetLabelTextProperty()->SetColor( 0.0, 0.0, 0.0 );
	m_sbActor->GetLabelTextProperty()->ShadowOff();
	m_sbActor->GetTitleTextProperty()->SetColor( 0.0, 0.0, 0.0 );
	m_sbActor->GetTitleTextProperty()->ShadowOff();
	m_sbActor->SetPosition( 0.0, 0.0 );
	m_sbActor->SetWidth( 1.0 );
	m_sbActor->SetHeight( 1.0 );
	m_sbActor->SetOrientationToHorizontal();
	m_sbActor->SetLookupTable( m_lut );
	m_sbActor->SetTitle( "Color Map" );
	m_sbActor->VisibilityOff();
	m_SBQVTKWidget->GetRenderWindow()->AddRenderer( m_sbRen );
	m_SBQVTKWidget->update();
	QVBoxLayout *lutLayoutHB = new QVBoxLayout( this );
	lutLayoutHB->setMargin( 0 );
	lutLayoutHB->addWidget( m_SBQVTKWidget );
	lutLayoutHB->update();
	scalarBarWidget->setLayout( lutLayoutHB );
}

void iASPMView::InitLUT()
{
	double lutRange[2] = { -0.13, 0.13 };
	m_lut->SetRange( lutRange );
	m_lut->Build();
	UpdateLUTOpacity();
}

iASPMView::~iASPMView()
{}

void iASPMView::SetData( const QTableWidget * data )
{
	//Init SPLOM
	m_splom->setData( data );

	m_splom->setSelectionColor(QColor(Qt::black));
	m_splom->setPointRadius(2.5);
	
	m_SPMSettings->parametersList->clear();
	m_SPMSettings->colorCodingParameter->clear();
	QString colorArName = defaultColorParam;
	m_updateColumnVisibility = false;
	for( int i = 0; i < data->columnCount(); ++i )
	{
		QString columnName = data->item( 0, i )->text();
		if ( columnName == "Mask Path" )	// No mask path option in settings dialog
			continue;
		QCheckBox * checkBox = new QCheckBox( columnName );
		QListWidgetItem * item = new QListWidgetItem( columnName, m_SPMSettings->parametersList );
		item->setFlags( item->flags() | Qt::ItemIsUserCheckable ); // set checkable flag
		item->setCheckState( Qt::Checked ); // AND initialize check state

		m_SPMSettings->colorCodingParameter->addItem( columnName );
	}
	m_updateColumnVisibility = true;
	m_SPMSettings->colorCodingParameter->setCurrentText( colorArName );
	m_sbActor->VisibilityOn();
}

void iASPMView::changeColumnVisibility( QListWidgetItem * item )
{
	if( !m_updateColumnVisibility )
		return;
	m_splom->setParameterVisibility( item->text(), item->checkState() );
}

inline void SetLookupTable( vtkPlotPoints * pp, vtkScalarsToColors * lut, const vtkStdString colorArrayName )
{
	pp->SetLookupTable( lut );
	pp->SelectColorArray( colorArrayName );
	pp->ScalarVisibilityOn();
}

void iASPMView::ApplyLookupTable()
{
	m_splom->setLookupTable( m_lut, m_colorArrayName );
	m_sbActor->SetLookupTable( m_lut );
	m_sbActor->SetTitle( m_colorArrayName.toStdString().data() );
	m_SBQVTKWidget->update();
}

void iASPMView::SetParameterToColorcode( const QString & paramName )
{
	if( !paramName.isEmpty() )
		m_colorArrayName = paramName;
	UpdateLookupTable();
}

void iASPMView::UpdateLookupTable()
{
	double lutRange[2] = { m_SPMSettings->sbMin->value(), m_SPMSettings->sbMax->value() };
	iAPerceptuallyUniformLUT::BuildPerceptuallyUniformLUT( m_lut, lutRange, 256 );
	UpdateLUTOpacity();
	ApplyLookupTable();
}

void iASPMView::selectionUpdated( QVector<unsigned int>* selInds )
{
	//selection
	m_SPLOMSelection = vtkSmartPointer<vtkIdTypeArray>::New();
	foreach( const unsigned int & i, *selInds )
		m_SPLOMSelection->InsertNextValue( vtkIdType( i ) );

	emit selectionModified( getActivePlotIndices(), m_SPLOMSelection );
}

void iASPMView::UpdateLUTOpacity()
{
	vtkIdType lutColCnt = m_lut->GetNumberOfTableValues();
	double alpha = (double)m_SPMSettings->opacitySlider->value() / m_SPMSettings->opacitySlider->maximum();
	for( vtkIdType i = 0; i < lutColCnt; i++ )
	{
		double rgba[4]; m_lut->GetTableValue( i, rgba );
		rgba[3] = alpha;
		m_lut->SetTableValue( i, rgba );
	}
	m_lut->Build();
}

void iASPMView::setSPLOMSelection( vtkIdTypeArray * ids )
{
	QVector<unsigned int> selInds;
	for( vtkIdType i = 0; i < ids->GetDataSize(); ++i )
		selInds.push_back( ids->GetValue( i ) );
	m_splom->setSelection( &selInds );
}

vtkVector2i iASPMView::getActivePlotIndices()
{
	int indices[2];
	m_splom->getActivePlotIndices( indices );
	return vtkVector2i( indices[0], indices[1] );
}

void iASPMView::setSelection( iASelection * sel )
{
	setSPLOMSelection( sel->ids );
	UpdateLookupTable();
}

void iASPMView::setDatasetsDir( QString datasetsDir )
{
	m_splom->setDatasetsDir( datasetsDir );
}

void iASPMView::setDatasetsByIndices( QStringList selDatasets, QList<int> indices )
{
	m_splom->setDatasetsByIndices( selDatasets, indices );
}

void iASPMView::reemitFixedPixmap()
{
	m_splom->reemitFixedPixmap();
}

void iASPMView::showSettings()
{
	m_SPMSettings->show();
}

void iASPMView::setRSDSelection( vtkIdTypeArray * rdsIds )
{
	setSPLOMSelection( rdsIds );
	emit selectionModified( getActivePlotIndices(), rdsIds );
}

void iASPMView::setSPLOMPreviewSliceNumbers( QList<int> sliceNumberLst )
{
	m_splom->setPreviewSliceNumbers( sliceNumberLst );
}

void iASPMView::setSPLOMPreviewSize( int percent )
{
	m_splom->settings.popupWidth = popupWidthRange[0] + (popupWidthRange[1] - popupWidthRange[0]) * percent/100.0;
	m_splom->update();
}

void iASPMView::setROIList( QList<QRectF> roi )
{
	m_splom->setROIList( roi );
}

void iASPMView::setSliceCnts( QList<int> sliceCnts )
{
	m_splom->setSliceCounts( sliceCnts );
}
