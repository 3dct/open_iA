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
#include "iAPDMView.h"

#include "iABPMData.h"
#include "iAHMData.h"

#include "defines.h"
#include "charts/qcustomplot.h"
#include "iAPerceptuallyUniformLUT.h"

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include <QVTKOpenGLWidget.h>
#else
#include <QVTKWidget.h>
#endif
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h> 
#include <vtkScalarBarActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTextProperty.h>

#include <algorithm>

void SetWidgetSelectionStyle(QWidget * w, bool isSelected)
{
	if( isSelected )
		w->setStyleSheet( "background-color:black;" );
	else
		w->setStyleSheet( "background-color:white;" );
}

iAPDMView::iAPDMView( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	: PorosityAnalyzerPDMConnector( parent, f ),
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	m_sbWiget( new QVTKOpenGLWidget( this ) ),
#else
	m_sbWiget( new QVTKWidget( this ) ),
#endif
	m_lut( vtkSmartPointer<vtkLookupTable>::New() ),
	m_sbRen( vtkSmartPointer<vtkRenderer>::New() ),
	m_sbActor( vtkSmartPointer<vtkScalarBarActor>::New() )
{
	QSettings settings( organisationName, applicationName );
	this->dsbCMRange->setValue( settings.value( "PorosityAnalyser/GUI/CMRange", 2.0 ).toDouble() );

	m_selectedIndices.clear();
	ShowDeviationControls( false );
	ShowPorosityRangeControls( false );
	iAPerceptuallyUniformLUT::BuildPerceptuallyUniformLUT( m_lut, -2.0, 2.0, 256 );
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
	m_sbActor->SetTitle( "Deviation from reference porosity (%)" );
	m_sbWiget->GetRenderWindow()->AddRenderer( m_sbRen );
	m_sbWiget->update();
	QVBoxLayout *lutLayoutHB = new QVBoxLayout( this );
	lutLayoutHB->setMargin( 0 );
	lutLayoutHB->addWidget( m_sbWiget );
	lutLayoutHB->update();
	scalarBarWidget->setLayout( lutLayoutHB );

	tableWidget->verticalHeader()->setVisible( true );
	tableWidget->horizontalHeader()->setVisible( true );
	cbRepresentation->setEnabled( false );
	connect( cbRepresentation, SIGNAL( currentIndexChanged( int ) ), this, SLOT( UpdateTable() ) );
	connect( cbTableFit, SIGNAL( currentIndexChanged( int ) ), this, SLOT( FitTable() ) );
	connect( cbPorosityRange, SIGNAL( currentIndexChanged( int ) ), this, SLOT( UpdateRepresentation() ) );
	connect( this->dsbCMRange, SIGNAL( valueChanged( double ) ), this, SLOT( UpdateColormapSettings( double ) ) );
}

iAPDMView::~iAPDMView()
{}

void iAPDMView::SetData( const iABPMData * bpData, const iAHMData * hmData )
{
	m_filters = &bpData->filters;
	m_datasets = &bpData->datasets;
	m_boxPlots = &bpData->boxPlots;

	m_histogramPlots = &hmData->histogramPlots;
	m_gtPorosityMap = &hmData->gtPorosityMap;
	cbRepresentation->setEnabled( true );
	m_indices.clear();
	UpdateTable();
}

void iAPDMView::UpdateTableDeviation()
{
	ShowDeviationControls( true );
	for( int i = 0; i < m_filters->size(); ++i )
		for( int j = 0; j < m_datasets->size(); ++j )
		{
			QWidget * wgt = new QWidget( 0 );
			
			QString datasetName = (*m_datasets)[j];
			double gt = (*m_gtPorosityMap).find( datasetName ).value();// Ground truth
			double med = (*m_boxPlots)[i][j].med;	//median value
			double deviation = med - gt;
			QString tip = "Dev: " + QString::number( deviation ) + " Med:" + QString::number( med ) + " Ref:" + QString::number( gt );
			double rgb[3];
			m_lut->GetColor( deviation, rgb );
			int irgb[3] = { static_cast<int>(rgb[0] * 255), static_cast<int>(rgb[1] * 255), static_cast<int>(rgb[2] * 255) };
			wgt->setStyleSheet( "background-color:rgb(" + QString::number( irgb[0] ) + "," + QString::number( irgb[1] ) + "," + QString::number( irgb[2] ) + ");" );
			wgt->setToolTip( tip );
			wgt->setStatusTip( tip );
			addWidgetToTable( j + 1, i + 1, wgt );
		}
}

void iAPDMView::UpdateTableBoxPlot()
{
	ShowPorosityRangeControls( true );
	for( int i = 0; i < m_filters->size(); ++i )
		for( int j = 0; j < m_datasets->size(); ++j )
		{
			QCustomPlot * customPlot = new QCustomPlot( 0 );
			// create empty statistical box plottables:
			QCPStatisticalBox *sample1 = new QCPStatisticalBox( customPlot->yAxis, customPlot->xAxis );
			QBrush boxBrush( QColor( 60, 60, 255, 100 ) );
			boxBrush.setStyle( Qt::Dense6Pattern ); // make it look oldschool
			sample1->setBrush( boxBrush );
			customPlot->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom );
			customPlot->setMinimumWidth( 250 );
			customPlot->setMinimumHeight( 80 );

			// set data:
			sample1->addData( 1, ( *m_boxPlots )[i][j].min,
				( *m_boxPlots )[i][j].q25, ( *m_boxPlots )[i][j].med,
				( *m_boxPlots )[i][j].q75, ( *m_boxPlots )[i][j].max,
				QVector<double>::fromList( ( *m_boxPlots )[i][j].outliers ) );

			// Fetch ground truth data
			QVector<double> valueData;
			QMap<double, QList<double> > map = (*m_histogramPlots)[i][j].histoBinMap;
			for( double idx = 0; idx < map.size(); ++idx )
				valueData << map.find( idx ).value().size();
			// Ground truth line
			QVector<double> xGroundTruth; QVector<double> yGroundTruth;
			QString datasetName = (*m_datasets)[j];
			xGroundTruth << (*m_gtPorosityMap).find( datasetName ).value();
			yGroundTruth << 1.4;
			QCPGraph * gtGraph = customPlot->addGraph();
			gtGraph->setData( xGroundTruth, yGroundTruth );
			gtGraph->setLineStyle( QCPGraph::lsImpulse );
			gtGraph->setPen( QPen( QColor( Qt::red ), 1.2, Qt::DashLine ) );
			gtGraph->setBrush( QBrush( QColor( 60, 60, 255, 100 ), Qt::Dense6Pattern ) );

			// prepare manual y axis labels:
			customPlot->yAxis->setTicks( false );
			customPlot->yAxis->setTickLabels( false );
			customPlot->yAxis->setVisible( false );

			// prepare axes:
			customPlot->xAxis->setLabel( "Porosity" );
			customPlot->rescaleAxes();
			customPlot->yAxis->scaleRange( 1.7, customPlot->yAxis->range().center() );
			QString str = cbPorosityRange->currentText();
			if ( !str.contains( "%" ) )	// ROI Porosity Range
			{
				double offset = ( *m_boxPlots )[i][j].range[1] - ( *m_boxPlots )[i][j].range[0]; offset *= 0.1;
				double xRange[2] = { ( *m_boxPlots )[i][j].range[0], ( *m_boxPlots )[i][j].range[1] };
				if ( xGroundTruth[0] < xRange[0] )
					xRange[0] = xGroundTruth[0];
				if ( xGroundTruth[0] > xRange[1] )
					xRange[1] = xGroundTruth[0];
				customPlot->xAxis->setRange( xRange[0] - offset, xRange[1] + offset );
			}
			else
			{
				customPlot->xAxis->setRange( 0.0, str.section( "%", 0, 0 ).toDouble() );
			}
			addWidgetToTable( j + 1, i + 1, customPlot );
		}
}

void iAPDMView::UpdateTableHistogram()
{
	ShowPorosityRangeControls( true );
	for ( int i = 0; i < m_filters->size(); ++i )
	{
		for ( int j = 0; j < m_datasets->size(); ++j )
		{
			// Fetch histogram data
			QVector<double> keyData; QVector<double> valueData;
			QMap<double, QList<double> > map = ( *m_histogramPlots )[i][j].histoBinMap;

			// prepare all x and y axis values 
			for ( double idx = 0; idx < map.size(); ++idx )
			{
				keyData << idx / 10.0;	// step size = 0.1; between 0 and 100 there are 1000 buckets
				valueData << map.find( idx ).value().size();
			}

			// get middle value (x axis) of histogram bukcet with higest frequency 
			int highestFrequency = *std::max_element( valueData.begin(), valueData.end() );
			double meanHigestHistoFrequencyBucket = 0.0;
			for ( int k = 0; k < valueData.size(); ++k )
			{
				//TODO: better mean calculation more buckets with slightly the same frequency
				if ( valueData.at( k ) == highestFrequency )
				{
					meanHigestHistoFrequencyBucket = k / 10.0 + 0.05;
					break;
				}
			}

			// Create histogram plot
			QCustomPlot * customPlot = new QCustomPlot( 0 );
			customPlot->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom );
			customPlot->setMinimumWidth( 250 );
			customPlot->setMinimumHeight( 120 );
			QCPGraph * mainGraph = customPlot->addGraph();
			mainGraph->setData( keyData, valueData );
			mainGraph->setLineStyle( QCPGraph::lsStepLeft );
			mainGraph->setPen( QPen( QColor( "#F59929" ), 1.8 ) );
			mainGraph->setBrush( QBrush( QColor( 60, 60, 255, 100 ), Qt::Dense6Pattern ) );

			// Ground truth line
			QVector<double> xGroundTruth; QVector<double> yGroundTruth;
			QString datasetName = ( *m_datasets )[j];
			xGroundTruth << ( *m_gtPorosityMap ).find( datasetName ).value();
			qSort( valueData.begin(), valueData.end() );
			yGroundTruth << valueData.last();
			QCPGraph * gtGraph = customPlot->addGraph();
			gtGraph->setData( xGroundTruth, yGroundTruth );
			gtGraph->setLineStyle( QCPGraph::lsImpulse );
			gtGraph->setPen( QPen( QColor( Qt::red ), 1.2, Qt::DashLine ) );
			gtGraph->setBrush( QBrush( QColor( 60, 60, 255, 100 ), Qt::Dense6Pattern ) );

			// prepare axes:
			customPlot->xAxis->setLabel( "Porosity" );
			customPlot->yAxis->setLabel( "Frequency" );
			customPlot->rescaleAxes();
			customPlot->yAxis->scaleRange( 1.7, customPlot->yAxis->range().center() );

			QString str = cbPorosityRange->currentText();
			if ( !str.contains( "%" ) )	// ROI Porosity Range
			{
				double offset = ( *m_histogramPlots )[i][j].range[1] - ( *m_histogramPlots )[i][j].range[0]; offset *= 0.1;
				double xRange[2] = { ( *m_histogramPlots )[i][j].range[0], ( *m_histogramPlots )[i][j].range[1] };
				if ( xGroundTruth[0] < xRange[0] )
					xRange[0] = xGroundTruth[0];
				if ( xGroundTruth[0] > xRange[1] )
					xRange[1] = xGroundTruth[0];
				customPlot->xAxis->setRange( xRange[0] - offset, xRange[1] + offset );
			}
			else
			{
				customPlot->xAxis->setRange( 0.0, str.section( "%", 0, 0 ).toDouble() );
			}
			addWidgetToTable( j + 1, i + 1, customPlot );
		}
	}
}

void iAPDMView::UpdateTable()
{
	tableWidget->setColumnCount( 0 );
	tableWidget->setRowCount( 0 );
	if( !m_filters->isEmpty() && !m_datasets->isEmpty() )
	{
		tableWidget->setColumnCount( m_filters->size() + 1 );
		tableWidget->setRowCount( m_datasets->size() + 1 );
		tableWidget->setItem( 0, 0, new QTableWidgetItem( "Filter/Dataset" ) );
	}
	for ( int i = 0; i < m_filters->size(); ++i )
	{
		QString rfn = ( *m_filters )[i];
		rfn.replace( "Create Surrounding", "CS" );
		rfn.replace( "Remove Surrounding", "RS" );
		QTableWidgetItem* twi = new QTableWidgetItem( rfn );
		twi->setToolTip( rfn );
		tableWidget->setItem( 0, i + 1, twi );
	}
	for( int i = 0; i < m_datasets->size(); ++i )
		tableWidget->setItem( i + 1, 0, new QTableWidgetItem( (*m_datasets)[i] ) );
	ShowDeviationControls( false );
	ShowPorosityRangeControls( false );
	UpdateRepresentation();
	m_selectedIndices.clear();
	FitTable();
	tableWidget->resizeRowsToContents();
}

void iAPDMView::FitTable()
{
	if( cbRepresentation->currentIndex() == 0 )
		switch( cbTableFit->currentIndex() )
		{
			case 0: //Fit to Text
				tableWidget->resizeColumnsToContents();
				break;
			case 1: //Minimize
				for( int nCol = 1; nCol < tableWidget->colorCount(); nCol++ )
					tableWidget->setColumnWidth( nCol, 24 );
				break;
		}
	else 
		tableWidget->resizeColumnsToContents();
}

void iAPDMView::UpdateRepresentation()
{
	switch ( cbRepresentation->currentIndex() )
	{
		case 0: //Deviation
			UpdateTableDeviation();
			break;
		case 1: //Box Plot
			UpdateTableBoxPlot();
			break;
		case 2: //Histogram
			UpdateTableHistogram();
			break;
	}
}

void iAPDMView::HighlightSelected( QObject * obj )
{
	QModelIndex index = m_indices[obj];
	bool isSelected = m_selectedIndices.contains( index );
	if( isSelected )
		m_selectedIndices.removeOne( index );
	else
		m_selectedIndices.push_back( index );
	SetWidgetSelectionStyle( (QWidget*)obj->parent(), !isSelected );
}

bool iAPDMView::eventFilter( QObject * obj, QEvent * event )
{
	if( event->type() == QEvent::MouseButtonRelease )
	{
		QMouseEvent * me = (QMouseEvent*)event;
		if( me->button() == Qt::LeftButton )
		{
			HighlightSelected( obj );
			emit selectionModified( m_selectedIndices );
		}
	}
	return false;
}

void iAPDMView::addWidgetToTable( int r, int c, QWidget * plot )
{
	QVBoxLayout * plotLayout = new QVBoxLayout();
	plotLayout->setMargin( 2 ); plotLayout->setSpacing( 0 );
	plotLayout->addWidget( plot );
	QWidget * plotWidget = new QWidget( this );
	plot->setParent( plotWidget );
	plotWidget->setLayout( plotLayout );

	tableWidget->setCellWidget( r, c, plotWidget );
	m_indices[plot] = tableWidget->model()->index( r, c );
	plot->installEventFilter( this );
}

void iAPDMView::setSelection( QModelIndexList selInds )
{
	m_selectedIndices = selInds;
	foreach( QModelIndex i, selInds )
	{	
		QWidget * w = (QWidget*)m_indices.key( i )->parent();
		SetWidgetSelectionStyle( w, true );
	}
}

void iAPDMView::ShowDeviationControls( bool visible )
{
	if( visible )
	{
		cmFrame->show();
		cbTableFit->show();
	}
	else
	{
		cmFrame->hide();
		cbTableFit->hide();
	}
}

void iAPDMView::ShowPorosityRangeControls( bool visible )
{
	if ( visible )
	{
		cbPorosityRange->show();
		cmFrame->hide();
		cbTableFit->hide();
	}
	else
	{
		cbPorosityRange->hide();
	}
}

void iAPDMView::UpdateColormapSettings( double range )
{
	iAPerceptuallyUniformLUT::BuildPerceptuallyUniformLUT( m_lut, -range, range, 256 );
	m_sbActor->SetLookupTable( m_lut );
	UpdateTableDeviation();
	m_sbWiget->update();

	QSettings settings( organisationName, applicationName );
	settings.setValue( "PorosityAnalyser/GUI/CMRange", range );
}
