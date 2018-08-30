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
#include "iALUT.h"
#include "iAPAQSplom.h"

#include "charts/iASPLOMData.h"
#include "iALookupTable.h"

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include <QVTKOpenGLWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#else
#include <QVTKWidget.h>
#endif
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

iASPMView::iASPMView(MainWindow *mWnd,  QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ ) : PorosityAnalyzerSPMConnector( parent, f ),
	m_SPLOMSelection( vtkSmartPointer<vtkIdTypeArray>::New() ),
	m_lut( vtkSmartPointer<vtkLookupTable>::New() ),
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	m_SBQVTKWidget( new QVTKOpenGLWidget( this ) ),
#else
	m_SBQVTKWidget( new QVTKWidget( this ) ),
#endif
	m_sbRen( vtkSmartPointer<vtkRenderer>::New() ),
	m_sbActor( vtkSmartPointer<vtkScalarBarActor>::New() ),
	m_splom( new iAPAQSplom(mWnd, parent) )
{
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	m_SBQVTKWidget->SetRenderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New());
#endif
	QHBoxLayout *layoutHB2 = new QHBoxLayout( this );
	layoutHB2->setMargin( 0 );
	layoutHB2->setSpacing( 0 );
	layoutHB2->addWidget( m_splom );
	SPLOMWidget->setLayout( layoutHB2 );

	initScalarBar();

	connect(tbSettings, SIGNAL(clicked()), m_splom, SLOT(showSettings()));

	connect( m_splom, &iAQSplom::selectionModified, this, &iASPMView::selectionUpdated );
	connect( m_splom, SIGNAL( previewSliceChanged( int ) ), this, SIGNAL( previewSliceChanged( int ) ) );
	connect( m_splom, SIGNAL( sliceCountChanged( int ) ), this, SIGNAL( sliceCountChanged( int ) ) );
	connect( m_splom, SIGNAL( maskHovered( const QPixmap *, int ) ), this, SIGNAL( maskHovered( const QPixmap *, int ) ) );
	connect( m_splom, SIGNAL( lookupTableChanged()), this, SLOT( applyLookupTable() ));
}

void iASPMView::initScalarBar()
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

iASPMView::~iASPMView()
{}

void iASPMView::setData( const QTableWidget * data )
{
	m_splom->setData( data );
	m_splom->setSelectionColor(QColor(Qt::black));
	m_splom->setPointRadius(2.5);
	m_splom->setColorParam(defaultColorParam);
	m_splom->settings.enableColorSettings = true;
	m_sbActor->VisibilityOn();
	applyLookupTable();
}

inline void SetLookupTable( vtkPlotPoints * pp, vtkScalarsToColors * lut, const vtkStdString colorArrayName )
{
	pp->SetLookupTable( lut );
	pp->SelectColorArray( colorArrayName );
	pp->ScalarVisibilityOn();
}

void iASPMView::applyLookupTable()
{
	updateLUT();
	m_sbActor->SetLookupTable( m_lut );
	m_sbActor->SetTitle( m_splom->data()->parameterName(m_splom->colorLookupParam()).toStdString().c_str() );
	m_SBQVTKWidget->update();
}

void iASPMView::selectionUpdated( std::vector<size_t> const & selInds )
{
	m_SPLOMSelection = vtkSmartPointer<vtkIdTypeArray>::New();
	for( auto & i: selInds )
		m_SPLOMSelection->InsertNextValue( static_cast<vtkIdType>( i ) );

	emit selectionModified( getActivePlotIndices(), m_SPLOMSelection );
}

void iASPMView::updateLUT()
{
	if (m_splom->lookupTable().numberOfValues() < m_lut->GetNumberOfTableValues())
		return;
	double rgba[4];
	vtkIdType lutColCnt = m_lut->GetNumberOfTableValues();
	m_lut->SetRange(m_splom->lookupTable().getRange());
	for( vtkIdType i = 0; i < lutColCnt; i++ )
	{
		m_splom->lookupTable().getTableValue(i, rgba);
		m_lut->SetTableValue( i, rgba );
	}
	m_lut->Build();
}

void iASPMView::setSPLOMSelection( vtkIdTypeArray * ids )
{
	iAQSplom::SelectionType selInds;
	for( vtkIdType i = 0; i < ids->GetDataSize(); ++i )
		selInds.push_back( ids->GetValue( i ) );
	m_splom->setSelection( selInds );
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
