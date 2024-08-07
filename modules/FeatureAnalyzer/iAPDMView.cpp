// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPDMView.h"

#include "iABPMData.h"
#include "iAHMData.h"

#include <qcustomplot.h>
#include <iALUT.h>
#include <iAQVTKWidget.h>

#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTextProperty.h>

#include <QGuiApplication>
#include <QSettings>

#include <algorithm>

void SetWidgetSelectionStyle(QWidget * w, bool isSelected)
{
	if (isSelected)
	{
		w->setStyleSheet("background-color:black;");
	}
	else
	{
		w->setStyleSheet("background-color:white;");
	}
}

iAPDMView::iAPDMView(QWidget* parent) :
	FeatureAnalyzerPDMConnector(parent),
	m_lut( vtkSmartPointer<vtkLookupTable>::New() ),
	m_sbWidget(new iAQVTKWidget()),
	m_sbRen( vtkSmartPointer<vtkRenderer>::New() ),
	m_sbActor(vtkSmartPointer<vtkScalarBarActor>::New())
{
	QSettings settings;
	this->dsbCMRange->setValue( settings.value( "FeatureAnalyzer/GUI/CMRange", 2.0 ).toDouble() );

	m_selectedIndices.clear();
	ShowDeviationControls( false );
	ShowPorosityRangeControls( false );
	iALUT::BuildLUT( m_lut, -2.0, 2.0, "Diverging blue-gray-red");
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
	m_sbWidget->renderWindow()->AddRenderer( m_sbRen );
	m_sbWidget->update();
	QVBoxLayout *lutLayoutHB = new QVBoxLayout( this );
	lutLayoutHB->setContentsMargins(0, 0, 0, 0);
	lutLayoutHB->addWidget( m_sbWidget );
	lutLayoutHB->update();
	scalarBarWidget->setLayout( lutLayoutHB );

	tableWidget->verticalHeader()->setVisible( true );
	tableWidget->horizontalHeader()->setVisible( true );
	cbRepresentation->setEnabled( false );
	connect( cbRepresentation, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAPDMView::UpdateTable);
	connect( cbTableFit, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAPDMView::FitTable);
	connect( cbPorosityRange, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAPDMView::UpdateRepresentation);
	connect( dsbCMRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &iAPDMView::UpdateColormapSettings);
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
	for (int i = 0; i < m_filters->size(); ++i)
	{
		for (int j = 0; j < m_datasets->size(); ++j)
		{
			auto wgt = new QWidget();

			QString datasetName = (*m_datasets)[j];
			double gt = (*m_gtPorosityMap).find(datasetName).value();// Ground truth
			double med = (*m_boxPlots)[i][j].med;	//median value
			double deviation = med - gt;
			QString tip = "Dev: " + QString::number(deviation) + " Med:" + QString::number(med) + " Ref:" + QString::number(gt);
			double rgb[3];
			m_lut->GetColor(deviation, rgb);
			int irgb[3] = { static_cast<int>(rgb[0] * 255), static_cast<int>(rgb[1] * 255), static_cast<int>(rgb[2] * 255) };
			wgt->setStyleSheet("background-color:rgb(" + QString::number(irgb[0]) + "," + QString::number(irgb[1]) + "," + QString::number(irgb[2]) + ");");
			wgt->setToolTip(tip);
			wgt->setStatusTip(tip);
			addWidgetToTable(j + 1, i + 1, wgt);
		}
	}
}

void iAPDMView::UpdateTableBoxPlot()
{
	ShowPorosityRangeControls( true );
	for (int i = 0; i < m_filters->size(); ++i)
	{
		for (int j = 0; j < m_datasets->size(); ++j)
		{
			auto customPlot = new QCustomPlot();
			// create empty statistical box plottables:
			QCPStatisticalBox* sample1 = new QCPStatisticalBox(customPlot->yAxis, customPlot->xAxis);
			QBrush boxBrush(QColor(60, 60, 255, 100));
			boxBrush.setStyle(Qt::Dense6Pattern); // make it look oldschool
			sample1->setBrush(boxBrush);
			customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
			customPlot->setMinimumWidth(250);
			customPlot->setMinimumHeight(80);

			// set data:
			sample1->addData(1, (*m_boxPlots)[i][j].min,
				(*m_boxPlots)[i][j].q25, (*m_boxPlots)[i][j].med,
				(*m_boxPlots)[i][j].q75, (*m_boxPlots)[i][j].max,
				QVector<double>::fromList((*m_boxPlots)[i][j].outliers));

			// Fetch ground truth data
			QVector<double> valueData;
			QMultiMap<double, QList<double> > map = (*m_histogramPlots)[i][j].histoBinMap;
			for (double idx = 0; idx < map.size(); ++idx)
			{
				valueData << map.find(idx).value().size();
			}
			// Ground truth line
			QVector<double> xGroundTruth; QVector<double> yGroundTruth;
			QString datasetName = (*m_datasets)[j];
			xGroundTruth << (*m_gtPorosityMap).find(datasetName).value();
			yGroundTruth << 1.4;
			QCPGraph* gtGraph = customPlot->addGraph();
			gtGraph->setData(xGroundTruth, yGroundTruth);
			gtGraph->setLineStyle(QCPGraph::lsImpulse);
			gtGraph->setPen(QPen(QColor(Qt::red), 1.2, Qt::DashLine));
			gtGraph->setBrush(QBrush(QColor(60, 60, 255, 100), Qt::Dense6Pattern));

			// prepare manual y axis labels:
			customPlot->yAxis->setTicks(false);
			customPlot->yAxis->setTickLabels(false);
			customPlot->yAxis->setVisible(false);

			// prepare axes:
			customPlot->xAxis->setLabel("Porosity");
			customPlot->rescaleAxes();
			customPlot->yAxis->scaleRange(1.7, customPlot->yAxis->range().center());
			QString str = cbPorosityRange->currentText();
			if (!str.contains("%"))	// ROI Porosity Range
			{
				double offset = (*m_boxPlots)[i][j].range[1] - (*m_boxPlots)[i][j].range[0]; offset *= 0.1;
				double xRange[2] = { (*m_boxPlots)[i][j].range[0], (*m_boxPlots)[i][j].range[1] };
				if (xGroundTruth[0] < xRange[0])
				{
					xRange[0] = xGroundTruth[0];
				}
				if (xGroundTruth[0] > xRange[1])
				{
					xRange[1] = xGroundTruth[0];
				}
				customPlot->xAxis->setRange(xRange[0] - offset, xRange[1] + offset);
			}
			else
			{
				customPlot->xAxis->setRange(0.0, str.section("%", 0, 0).toDouble());
			}
			addWidgetToTable(j + 1, i + 1, customPlot);
		}
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
			QMultiMap<double, QList<double> > map = ( *m_histogramPlots )[i][j].histoBinMap;

			// prepare all x and y axis values
			for ( double idx = 0; idx < map.size(); ++idx )
			{
				keyData << idx / 10.0;	// step size = 0.1; between 0 and 100 there are 1000 buckets
				valueData << map.find( idx ).value().size();
			}

			// NOT USED - REMOVE?
			/*
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
			*/

			// Create histogram plot
			auto customPlot = new QCustomPlot();
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
			std::sort( valueData.begin(), valueData.end() );
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
				if (xGroundTruth[0] < xRange[0])
				{
					xRange[0] = xGroundTruth[0];
				}
				if (xGroundTruth[0] > xRange[1])
				{
					xRange[1] = xGroundTruth[0];
				}
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
		tableWidget->setColumnCount( static_cast<int>(m_filters->size() + 1) );
		tableWidget->setRowCount( static_cast<int>(m_datasets->size() + 1) );
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
	for (int i = 0; i < m_datasets->size(); ++i)
	{
		tableWidget->setItem(i + 1, 0, new QTableWidgetItem((*m_datasets)[i]));
	}
	ShowDeviationControls( false );
	ShowPorosityRangeControls( false );
	UpdateRepresentation();
	m_selectedIndices.clear();
	FitTable();
	tableWidget->resizeRowsToContents();
}

void iAPDMView::FitTable()
{
	if (cbRepresentation->currentIndex() == 0)
	{
		switch (cbTableFit->currentIndex())
		{
		case 0: //Fit to Text
			tableWidget->resizeColumnsToContents();
			break;
		case 1: //Minimize
			for (int nCol = 1; nCol < tableWidget->colorCount(); nCol++)
				tableWidget->setColumnWidth(nCol, 24);
			break;
		}
	}
	else
	{
		tableWidget->resizeColumnsToContents();
	}
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
	if (isSelected)
	{
		m_selectedIndices.removeOne(index);
	}
	else
	{
		m_selectedIndices.push_back(index);
	}
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
	plotLayout->setContentsMargins(2, 2, 2, 2);
	plotLayout->setSpacing(0);
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
	for (QModelIndex i: selInds )
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
	iALUT::BuildLUT( m_lut, -range, range, "Diverging blue-gray-red" );
	m_sbActor->SetLookupTable( m_lut );
	UpdateTableDeviation();
	m_sbWidget->update();

	QSettings settings;
	settings.setValue( "FeatureAnalyzer/GUI/CMRange", range );
}
