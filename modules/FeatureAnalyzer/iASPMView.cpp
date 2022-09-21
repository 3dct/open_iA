/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAFAQSplom.h"
#include "iASelection.h"
#include "FeatureAnalyzerHelpers.h"

#include <iASPLOMData.h>
#include <iALookupTable.h>
#include <iALUT.h>
#include <iAQtVTKBindings.h>
#include <iAQVTKWidget.h>

#include <vtkIdTypeArray.h>
#include <vtkLookupTable.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>

#include <QCheckBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QMenu>
#include <QTableWidget>
#include <QtGlobal>
#include <QVBoxLayout>


const QString defaultColorParam = "Deviat. from Ref.";
const int popupWidthRange[2] = { 80, 300 };

iASPMView::iASPMView(iAMainWindow* mWnd, QWidget* parent):
	iAPorosityAnalyzerSPMConnector(parent),
	m_splom(new iAFAQSplom(mWnd, parent)),
	m_SPLOMSelection( vtkSmartPointer<vtkIdTypeArray>::New() ),
	m_lut( vtkSmartPointer<vtkLookupTable>::New() ),
	m_SBQVTKWidget(new iAQVTKWidget()),
	m_sbRen( vtkSmartPointer<vtkRenderer>::New() ),
	m_sbActor(vtkSmartPointer<vtkScalarBarActor>::New())
{
	QHBoxLayout *layoutHB2 = new QHBoxLayout( this );
	layoutHB2->setContentsMargins(0, 0, 0, 0);
	layoutHB2->setSpacing( 0 );
	layoutHB2->addWidget( m_splom );
	SPLOMWidget->setLayout( layoutHB2 );

	initScalarBar();

	connect(tbSettings, &QToolButton::clicked, m_splom, &iAFAQSplom::showSettings);

	connect(m_splom, &iAQSplom::selectionModified, this, &iASPMView::selectionUpdated );
	connect(m_splom, &iAFAQSplom::previewSliceChanged, this, &iASPMView::previewSliceChanged);
	connect(m_splom, &iAFAQSplom::sliceCountChanged, this, &iASPMView::sliceCountChanged);
	connect(m_splom, &iAFAQSplom::maskHovered, this, &iASPMView::maskHovered);
	connect(m_splom, &iAQSplom::lookupTableChanged, this, &iASPMView::applyLookupTable);
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
	m_SBQVTKWidget->renderWindow()->AddRenderer( m_sbRen );
	m_SBQVTKWidget->update();
	QVBoxLayout *lutLayoutHB = new QVBoxLayout( this );
	lutLayoutHB->setContentsMargins(0, 0, 0, 0);
	lutLayoutHB->addWidget( m_SBQVTKWidget );
	lutLayoutHB->update();
	scalarBarWidget->setLayout( lutLayoutHB );
}

iASPMView::~iASPMView()
{}

void iASPMView::setData( const QTableWidget * newData )
{
	m_splom->setData( newData );
	m_splom->setSelectionColor(QColor(Qt::black));
	m_splom->setPointRadius(2.5);
	m_splom->setColorParam(defaultColorParam);
	m_splom->settings.enableColorSettings = true;
	m_sbActor->VisibilityOn();
	applyLookupTable();
}

void iASPMView::applyLookupTable()
{
	if (m_splom->data()->numParams() == 0)
	{
		return;
	}
	updateLUT();
	m_sbActor->SetLookupTable( m_lut );
	m_sbActor->SetTitle( m_splom->data()->parameterName(m_splom->colorLookupParam()).toStdString().c_str() );
	m_SBQVTKWidget->renderWindow()->Render();
	m_SBQVTKWidget->update();
}

void iASPMView::selectionUpdated( std::vector<size_t> const & selInds )
{
	m_SPLOMSelection = vtkSmartPointer<vtkIdTypeArray>::New();
	for (auto& i : selInds)
	{
		m_SPLOMSelection->InsertNextValue(static_cast<vtkIdType>(i));
	}

	emit selectionModified( getActivePlotIndices(), m_SPLOMSelection );
}

void iASPMView::updateLUT()
{
	if (m_splom->lookupTable()->numberOfValues() < static_cast<size_t>(m_lut->GetNumberOfTableValues()))
	{
		return;
	}
	double rgba[4];
	vtkIdType lutColCnt = m_lut->GetNumberOfTableValues();
	m_lut->SetRange(m_splom->lookupTable()->getRange());
	for( vtkIdType i = 0; i < lutColCnt; i++ )
	{
		m_splom->lookupTable()->getTableValue(i, rgba);
		m_lut->SetTableValue( i, rgba );
	}
	m_lut->Build();
}

void iASPMView::setSPLOMSelection( vtkIdTypeArray * ids )
{
	iAScatterPlotViewData::SelectionType selInds;
	for (vtkIdType i = 0; i < ids->GetDataSize(); ++i)
	{
		selInds.push_back(ids->GetValue(i));
	}
	m_splom->viewData()->setSelection(selInds);
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
