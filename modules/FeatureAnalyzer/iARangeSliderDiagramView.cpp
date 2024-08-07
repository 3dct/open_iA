// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARangeSliderDiagramView.h"

#include "iARangeSliderDiagramWidget.h"
#include "FeatureAnalyzerHelpers.h"

#include <iAHistogramData.h>
#include <iAMathUtility.h>

#include <vtkIdTypeArray.h>

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
std::shared_ptr<iAHistogramData> createRangeSliderData(QList<double> m_rangeSliderData, double min, double max)
{
	auto data = iAHistogramData::create("Frequency", iAValueType::Continuous, min, max, m_rangeSliderData.size());
	size_t idx = 0;
	for (double val : m_rangeSliderData)
	{
		data->setBin(idx, val);
		++idx;
	}
	return data;
}
}

iARangeSliderDiagramView::iARangeSliderDiagramView(QWidget * parent, Qt::WindowFlags f):
	QWidget( parent, f ),
	m_comboBoxContainer(nullptr),
	m_histoContainer(nullptr),
	m_layoutVBMainContainer(nullptr),
	m_layoutHBComboBoxes(nullptr),
	m_layoutVBHistoContainer(nullptr),
	m_cbPorDev(nullptr),
	m_cbStatisticMeasurements(nullptr),
	m_title(nullptr),
	m_input(nullptr),
	m_output(nullptr),
	m_separator(nullptr)
{
}

void iARangeSliderDiagramView::setData( const QTableWidget * newData )
{
	m_rawTable = newData;

	deleteOutdated();
	addTitleLabel();
	setupHistogram();
	addComboBoxes();
	setupDiagrams();

	setLayout( m_layoutVBMainContainer );
	show();
}

QMultiMap<QString, QList<double> > iARangeSliderDiagramView::prepareData( const QTableWidget * tableData, bool porOrDev, bool statisticMeasurements )
{
	QMultiMap<QString, QList<double> > map;
	int NbOfParam = tableData->columnCount() - 4;

	// Traverse all parameters
	for ( int param = 0; param < NbOfParam; ++param )
	{
		//Get the individual parameters as a sorted list
		QList<double> intervals;
		for ( int i = 1; i < tableData->rowCount(); ++i )
		{
			double newInterval = tableData->item( i, param )->text().toDouble();
			if (!intervals.contains(newInterval))
			{
				intervals.append(newInterval);
			}
		}

		//Calculate the median or mean for each interval and store it in a list
		QList<double> meanValues;
		QListIterator<double> intervalsIt( intervals );
		while ( intervalsIt.hasNext() )
		{
			std::vector<double> unsortedValues;
			for ( int j = 1; j < tableData->rowCount(); ++j )
			{
				if (tableData->item(j, param)->text().toDouble() == intervalsIt.peekNext())
				{
					unsortedValues.push_back(tableData->item(j, NbOfParam + porOrDev)->text().toDouble());
				}
			}
			intervalsIt.next();
			if (statisticMeasurements)
			{
				meanValues.append(mean(unsortedValues));
			}
			else
			{
				meanValues.append(median(unsortedValues));
			}
		}
		map.insert(tableData->item( 0, param )->text(), meanValues );
	}
	return map;
}

void iARangeSliderDiagramView::updateDiagrams()
{
	QListIterator<iARangeSliderDiagramWidget *> widgetIt( m_widgetList );
	widgetIt.next();
	while ( widgetIt.hasNext() )
	{
		delete widgetIt.next();
		m_widgetList.removeAt( 1 );
		m_oTFList.removeAt( 1 );
		m_cTFList.removeAt( 1 );
	}
	setupDiagrams();
	m_widgetList.at( 0 )->updateSelectedDiagrams();
}

void iARangeSliderDiagramView::addTitleLabel()
{
	//Setup main container for GUI elements
	setMinimumHeight( 300 );
	m_layoutVBMainContainer = new QVBoxLayout( this );
	m_layoutVBMainContainer->setContentsMargins(0, 0, 0, 0);

	//Install event filter for main conatiner
	installEventFilter( this );

	// Title label
	m_title = new QLabel();
	QFont titleFont = m_title->font();
	m_title->setStyleSheet( "QLabel { background-color: lightGray }" );
	titleFont.setPointSize( 11 );
	titleFont.setBold( true );
	m_title->setFont( titleFont );
	m_title->setMargin( 0 );
	m_title->setFixedHeight( 22 );
	m_title->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	m_title->setText( tr( "Algorithm '%1' performed on dataset '%2'" )
					  .arg( m_rawTable->item( 1, m_rawTable->columnCount() - 1 )->text() )
					  .arg( m_rawTable->item( 1, m_rawTable->columnCount() - 2 )->text() ) );
	m_title->setAlignment( Qt::AlignVCenter | Qt::AlignHCenter );
	m_layoutVBMainContainer->addWidget( m_title );

	// Output label
	m_output = new QLabel();
	QFont outputFont = m_output->font();
	m_output->setStyleSheet( "QLabel { background-color: rgba(0,0,0,0%) }" );
	outputFont.setPointSize( 11 );
	outputFont.setBold( true );
	m_output->setFont( outputFont );
	m_output->setMargin( 0 );
	m_output->setFixedHeight( 22 );
	m_output->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	m_output->setText( "<font color = 'darkRed'>Output Derivative< / font>" );
	m_output->setAlignment( Qt::AlignTop | Qt::AlignCenter );
	m_layoutVBMainContainer->addWidget( m_output );
}

void iARangeSliderDiagramView::addComboBoxes()
{
	// Separator
	m_separator = new QFrame();
	m_separator->setGeometry( QRect( 320, 150, 118, 3 ) );
	m_separator->setFrameShape( QFrame::HLine );
	m_separator->setStyleSheet( "border-width: 1px; border-top-style: none; border-right-style: none;"
						 "border-bottom-style: solid; border-left-style: none; border-color: lightGray; " );
	m_separator->setFrameShadow( QFrame::Sunken );
	m_layoutVBMainContainer->addWidget( m_separator );

	// Input label
	m_input = new QLabel();
	QFont font = m_input->font();
	m_input->setStyleSheet( "QLabel { background-color: rgba(0,0,0,0%) }" );
	font.setPointSize( 11 );
	font.setBold( true );
	m_input->setFont( font );
	m_input->setMargin( 0 );
	m_input->setFixedHeight( 22 );
	m_input->setFrameStyle( QFrame::Panel | QFrame::Sunken );
	m_input->setText( "<font color = 'darkRed'>Input Parameters< / font>" );
	m_input->setAlignment( Qt::AlignTop | Qt::AlignCenter );
	m_layoutVBMainContainer->addWidget( m_input );

	// ComboBoxes
	m_comboBoxContainer = new QWidget();
	m_comboBoxContainer->setFixedHeight( 25 );
	m_comboBoxContainer->setFixedWidth( 200 );
	m_layoutHBComboBoxes = new QHBoxLayout(this);
	m_layoutHBComboBoxes->setContentsMargins(0, 0, 0, 0);
	m_layoutHBComboBoxes->setContentsMargins( 0, 0, 0, 0 );

	m_cbPorDev = new QComboBox();
	m_cbPorDev->addItem( m_rawTable->item( 0, m_rawTable->columnCount() - 4 )->text() ); // Porosity
	m_cbPorDev->addItem( m_rawTable->item( 0, m_rawTable->columnCount() - 3 )->text().append( " from ref." ) ); //Deviation
	QSizePolicy comboBoxSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	comboBoxSizePolicy.setHorizontalStretch( 0 );
	comboBoxSizePolicy.setVerticalStretch( 0 );
	comboBoxSizePolicy.setHeightForWidth( m_cbPorDev->sizePolicy().hasHeightForWidth() );
	connect(m_cbPorDev, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iARangeSliderDiagramView::updateDiagrams);
	m_cbPorDev->setSizePolicy( comboBoxSizePolicy );
	m_layoutHBComboBoxes->addWidget( m_cbPorDev );

	m_cbStatisticMeasurements = new QComboBox();
	m_cbStatisticMeasurements->addItem( "Median" );
	m_cbStatisticMeasurements->addItem( "Mean" );
	connect( m_cbStatisticMeasurements, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iARangeSliderDiagramView::updateDiagrams);
	m_cbStatisticMeasurements->setSizePolicy( comboBoxSizePolicy );
	m_layoutHBComboBoxes->addWidget( m_cbStatisticMeasurements );

	m_comboBoxContainer->setLayout( m_layoutHBComboBoxes );
	m_layoutVBMainContainer->addWidget( m_comboBoxContainer );
	m_layoutVBMainContainer->addSpacing( 5 );
}

void iARangeSliderDiagramView::loadSelectionToSPMView()
{
	QSet<int> rsdSelection;
	for ( int i = 1; i < m_widgetList.size(); ++i )
	{
		if ( m_widgetList[i]->getSelectedRawTableRows().size() != 0 )
		{
			if ( rsdSelection.count() )
			{
				auto l = m_widgetList[i]->getSelectedRawTableRows();
				rsdSelection.intersect(QSet<int>(l.begin(), l.end()));
			}
			else
			{
				auto l = m_widgetList[i]->getSelectedRawTableRows();
				rsdSelection.unite(QSet<int>(l.begin(), l.end()));
			}
		}
	}

	QList<int> rsdSelectionList(rsdSelection.values());
	std::sort( rsdSelectionList.begin(), rsdSelectionList.end() );

	vtkIdTypeArray *rdsIds = vtkIdTypeArray::New();
	rdsIds->SetName( "rsdIds" );
	for (int i = 0; i < rsdSelectionList.size(); ++i)
	{
		rdsIds->InsertNextValue(rsdSelectionList[i]);
	}

	emit selectionModified( rdsIds );
}

void iARangeSliderDiagramView::deleteOutdated()
{
	//Delete old diagrams and GUI elements
	QListIterator<iARangeSliderDiagramWidget *> widgetIt( m_widgetList );
	while (widgetIt.hasNext())
	{
		delete widgetIt.next();
	}
	m_widgetList.clear();
	m_oTFList.clear();
	m_cTFList.clear();
	delete m_title; m_title = nullptr;
	delete m_cbPorDev; m_cbPorDev = nullptr;
	delete m_cbStatisticMeasurements; m_cbStatisticMeasurements = nullptr;
	delete m_layoutHBComboBoxes; m_layoutHBComboBoxes = nullptr;
	delete m_comboBoxContainer; m_comboBoxContainer = nullptr;
	delete m_layoutVBHistoContainer; m_layoutVBHistoContainer = nullptr;
	delete m_histoContainer; m_histoContainer = nullptr;
	delete m_separator; m_separator = nullptr;
	delete m_input; m_input = nullptr;
	delete m_output; m_output = nullptr;
	delete m_layoutVBMainContainer; m_layoutVBMainContainer = nullptr;
	hide();
}

void iARangeSliderDiagramView::setupHistogram()
{
	//Setup frequency/porosity histogram (OUTPUT)
	QList<double> porosityList;
	for (int i = 1; i < m_rawTable->rowCount(); ++i)
	{
		porosityList.append(m_rawTable->item(i, m_rawTable->columnCount() - 4)->text().toDouble());
	}

	QList<double> binList;
	m_histogramMap = calculateHistogram( porosityList, 0.0, 100.0 );
	QMultiMapIterator<double, QList<double>> mapIt(m_histogramMap);
	while ( mapIt.hasNext() )
	{
		mapIt.next();
		binList.append( mapIt.value().size() );
	}

	m_rangeSliderData = createRangeSliderData( binList, 0.0, 99.9);

	m_rangeSliderDiagramDrawer = std::make_shared<iABarGraphPlot>(m_rangeSliderData, QColor(70, 70, 70, 255));
	auto oTF = vtkSmartPointer<vtkPiecewiseFunction>::New();
	auto cTF = vtkSmartPointer<vtkColorTransferFunction>::New();
	// Adds two end points to set up a propper transfer function
	oTF->AddPoint( m_rangeSliderData->xBounds()[0], 0 );
	oTF->AddPoint( m_rangeSliderData->xBounds()[1], 0 );
	cTF->AddRGBPoint( m_rangeSliderData->xBounds()[0], 0, 0, 0 );
	cTF->AddRGBPoint( m_rangeSliderData->xBounds()[1], 0, 0, 0 );
	m_oTFList.append( oTF );
	m_cTFList.append( cTF );

	auto *rangeSliderDiagramWidget = new iARangeSliderDiagramWidget( dynamic_cast<QWidget*> ( parent() ), m_oTFList.at( 0 ),
		m_cTFList.at( 0 ), m_rangeSliderData, &m_histogramMap, m_rawTable,"Porosity", "Frequency" );

	rangeSliderDiagramWidget->addPlot( m_rangeSliderDiagramDrawer );
	m_widgetList.append( rangeSliderDiagramWidget );
	m_layoutVBMainContainer->addWidget( rangeSliderDiagramWidget );
}

void iARangeSliderDiagramView::setupDiagrams()
{
	//Setup parameter diagrams (INPUT)
	auto paramPorOrDevMap = prepareData( m_rawTable, m_cbPorDev->currentIndex(),
																  m_cbStatisticMeasurements->currentIndex() );
	QMultiMapIterator<QString, QList<double>> mapIt(paramPorOrDevMap);
	while ( mapIt.hasNext() )
	{
		mapIt.next();

		int paramColumnPos = 0;	/*param position in column*/
		for ( int i = 0; i < m_rawTable->columnCount(); ++i )
		{
			if (mapIt.key() == m_rawTable->item(0, i)->text())
			{
				paramColumnPos = i;
			}
		}

		double min = m_rawTable->item( 1, paramColumnPos )->text().toDouble();
		double max = m_rawTable->item( m_rawTable->rowCount() - 1, paramColumnPos )->text().toDouble();
		m_rangeSliderData = createRangeSliderData(mapIt.value(), min, max);
		m_rangeSliderDiagramDrawer = std::make_shared<iABarGraphPlot>(m_rangeSliderData, QColor(70, 70, 70, 255));
		auto oTF = vtkSmartPointer<vtkPiecewiseFunction>::New();
		auto cTF = vtkSmartPointer<vtkColorTransferFunction>::New();
		// Adds two end points to set up a propper transfer function
		oTF->AddPoint( m_rangeSliderData->xBounds()[0], 0 );
		oTF->AddPoint( m_rangeSliderData->xBounds()[1], 0 );
		cTF->AddRGBPoint( m_rangeSliderData->xBounds()[0], 0, 0, 0 );
		cTF->AddRGBPoint( m_rangeSliderData->xBounds()[1], 0, 0, 0 );
		m_oTFList.append( oTF );
		m_cTFList.append( cTF );

		auto * rangeSliderDiagramWidget = new iARangeSliderDiagramWidget(dynamic_cast<QWidget*>(parent()),
				m_oTFList.last(), m_cTFList.last(), m_rangeSliderData, &m_histogramMap, m_rawTable, mapIt.key(), m_cbPorDev->currentText() );

		connect( m_widgetList[0], &iARangeSliderDiagramWidget::selected, rangeSliderDiagramWidget,    &iARangeSliderDiagramWidget::selectSlot);
		connect( m_widgetList[0], &iARangeSliderDiagramWidget::deselected, rangeSliderDiagramWidget,  &iARangeSliderDiagramWidget::deleteSlot);
		connect( rangeSliderDiagramWidget, &iARangeSliderDiagramWidget::selectionRelesedSignal, this, &iARangeSliderDiagramView::loadSelectionToSPMView);

		rangeSliderDiagramWidget->addPlot( m_rangeSliderDiagramDrawer );
		m_widgetList.append( rangeSliderDiagramWidget );
		m_layoutVBMainContainer->addWidget( rangeSliderDiagramWidget );
	}
}

void iARangeSliderDiagramView::clearOldRSDView()
{
	if (m_mainContainer)
	{
		deleteOutdated();
	}
}
