/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iARangeSliderDiagramView.h"

#include "iARangeSliderDiagramData.h"
#include "iARangeSliderDiagramWidget.h"
#include "PorosityAnalyserHelpers.h"

#include <vtkIdTypeArray.h>

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QTableWidget>

iARangeSliderDiagramView::iARangeSliderDiagramView( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	: RangeSliderDiagramViewConnector( parent, f ), 
	m_mainContainer( 0 ), m_layoutVBMainContainer( 0 ),
	m_cbPorDev( 0 ), 
	m_cbStatisticMeasurements( 0 ), 
	m_layoutHBComboBoxes( 0 ), 
	m_comboBoxContainer( 0 ), 
	m_title( 0 ), 
	m_histoContainer( 0 ),
	m_layoutVBHistoContainer( 0 ),
	m_separator( 0 ),
	m_input( 0 ),
	m_output( 0 )
{
}

iARangeSliderDiagramView::~iARangeSliderDiagramView()
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
	
	m_mainContainer->setLayout( m_layoutVBMainContainer );
	rangeSliderDiagramContainer->addWidget( m_mainContainer );
	show();
}

QMap<QString, QList<double> > iARangeSliderDiagramView::prepareData( const QTableWidget * tableData, bool porOrDev, bool statisticMeasurements )
{
	QMap<QString, QList<double> > map;
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
		map.insertMulti(tableData->item( 0, param )->text(), meanValues );
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
	m_mainContainer = new QWidget();
	m_mainContainer->setMinimumHeight( 300 );
	m_layoutVBMainContainer = new QVBoxLayout( this );
	m_layoutVBMainContainer->setMargin( 0 );

	//Install event filter for main conatiner
	m_mainContainer->installEventFilter( this );

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
	m_layoutHBComboBoxes->setMargin( 0 );
	m_layoutHBComboBoxes->setContentsMargins( 0, 0, 0, 0 );

	m_cbPorDev = new QComboBox();
	m_cbPorDev->addItem( m_rawTable->item( 0, m_rawTable->columnCount() - 4 )->text() ); // Porosity
	m_cbPorDev->addItem( m_rawTable->item( 0, m_rawTable->columnCount() - 3 )->text().append( " from ref." ) ); //Deviation
	QSizePolicy comboBoxSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	comboBoxSizePolicy.setHorizontalStretch( 0 );
	comboBoxSizePolicy.setVerticalStretch( 0 );
	comboBoxSizePolicy.setHeightForWidth( m_cbPorDev->sizePolicy().hasHeightForWidth() );
	connect( m_cbPorDev, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateDiagrams() ) );
	m_cbPorDev->setSizePolicy( comboBoxSizePolicy );
	m_layoutHBComboBoxes->addWidget( m_cbPorDev );

	m_cbStatisticMeasurements = new QComboBox();
	m_cbStatisticMeasurements->addItem( "Median" ); 
	m_cbStatisticMeasurements->addItem( "Mean" ); 
	connect( m_cbStatisticMeasurements, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateDiagrams() ) );
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
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
				auto l = m_widgetList[i]->getSelectedRawTableRows();
				rsdSelection.intersect(QSet<int>(l.begin(), l.end()));
#else
				rsdSelection.intersect( m_widgetList[i]->getSelectedRawTableRows().toSet() );
#endif
			}
			else
			{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
				auto l = m_widgetList[i]->getSelectedRawTableRows();
				rsdSelection.unite(QSet<int>(l.begin(), l.end()));
#else
				rsdSelection.unite( m_widgetList[i]->getSelectedRawTableRows().toSet() );
#endif
			}
		}
	}

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	QList<int> rsdSelectionList(rsdSelection.values());
#else
	QList<int> rsdSelectionList = rsdSelection.toList();
#endif
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
	delete m_title; m_title = 0;
	delete m_cbPorDev; m_cbPorDev = 0;
	delete m_cbStatisticMeasurements; m_cbStatisticMeasurements = 0;
	delete m_layoutHBComboBoxes; m_layoutHBComboBoxes = 0;
	delete m_comboBoxContainer; m_comboBoxContainer = 0;
	delete m_layoutVBHistoContainer; m_layoutVBHistoContainer = 0;
	delete m_histoContainer; m_histoContainer = 0;
	delete m_separator; m_separator = 0;
	delete m_input; m_input = 0;
	delete m_output; m_output = 0;
	delete m_layoutVBMainContainer; m_layoutVBMainContainer = 0;
	delete m_mainContainer; m_mainContainer = 0;
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
	QMapIterator<double, QList<double> > mapIt( m_histogramMap );
	while ( mapIt.hasNext() )
	{
		mapIt.next();
		binList.append( mapIt.value().size() );
	}

	m_rangeSliderData = QSharedPointer<iARangeSliderDiagramData>( new iARangeSliderDiagramData( binList, 0.0, 99.9 ) );
	m_rangeSliderData->updateRangeSliderFunction();

	m_rangeSliderDiagramDrawer = QSharedPointer<iABarGraphPlot>( new iABarGraphPlot( m_rangeSliderData, QColor(70, 70, 70, 255)) );
	vtkSmartPointer<vtkPiecewiseFunction> oTF = vtkSmartPointer<vtkPiecewiseFunction>::New();
	vtkSmartPointer<vtkColorTransferFunction> cTF = vtkSmartPointer<vtkColorTransferFunction>::New();
	// Adds two end points to set up a propper transfer function
	oTF->AddPoint( m_rangeSliderData->xBounds()[0], 0 );
	oTF->AddPoint( m_rangeSliderData->xBounds()[1], 0 );
	cTF->AddRGBPoint( m_rangeSliderData->xBounds()[0], 0, 0, 0 );
	cTF->AddRGBPoint( m_rangeSliderData->xBounds()[1], 0, 0, 0 );
	m_oTFList.append( oTF );
	m_cTFList.append( cTF );

	iARangeSliderDiagramWidget *rangeSliderDiagramWidget = new iARangeSliderDiagramWidget( dynamic_cast<QWidget*> ( parent() ), nullptr, m_oTFList.at( 0 ),
													 m_cTFList.at( 0 ), m_rangeSliderData, &m_histogramMap, m_rawTable,"Porosity", "Frequency" );

	rangeSliderDiagramWidget->addPlot( m_rangeSliderDiagramDrawer );
	m_widgetList.append( rangeSliderDiagramWidget );
	m_layoutVBMainContainer->addWidget( rangeSliderDiagramWidget );
}

void iARangeSliderDiagramView::setupDiagrams()
{
	//Setup parameter diagrams (INPUT)
	QMap<QString, QList<double> > paramPorOrDevMap = prepareData( m_rawTable, m_cbPorDev->currentIndex(),
																  m_cbStatisticMeasurements->currentIndex() );

	
	QMapIterator<QString, QList<double> > mapIt( paramPorOrDevMap );
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
		m_rangeSliderData = QSharedPointer<iARangeSliderDiagramData>( new iARangeSliderDiagramData( mapIt.value(), min, max ) );

		m_rangeSliderData->updateRangeSliderFunction();

		m_rangeSliderDiagramDrawer = QSharedPointer<iABarGraphPlot>( new iABarGraphPlot( m_rangeSliderData, QColor(70, 70, 70, 255)) );
		vtkSmartPointer<vtkPiecewiseFunction> oTF = vtkSmartPointer<vtkPiecewiseFunction>::New();
		vtkSmartPointer<vtkColorTransferFunction> cTF = vtkSmartPointer<vtkColorTransferFunction>::New();;
		// Adds two end points to set up a propper transfer function
		oTF->AddPoint( m_rangeSliderData->xBounds()[0], 0 );
		oTF->AddPoint( m_rangeSliderData->xBounds()[1], 0 );
		cTF->AddRGBPoint( m_rangeSliderData->xBounds()[0], 0, 0, 0 );
		cTF->AddRGBPoint( m_rangeSliderData->xBounds()[1], 0, 0, 0 );
		m_oTFList.append( oTF );
		m_cTFList.append( cTF );

		iARangeSliderDiagramWidget* rangeSliderDiagramWidget = new iARangeSliderDiagramWidget( dynamic_cast<QWidget*> ( parent() ), nullptr,
															m_oTFList.last(), m_cTFList.last(), m_rangeSliderData, 
															&m_histogramMap, m_rawTable, mapIt.key(), m_cbPorDev->currentText() );

		connect( m_widgetList[0], SIGNAL( selected() ), rangeSliderDiagramWidget, SLOT( selectSlot() ) );
		connect( m_widgetList[0], SIGNAL( deselected() ), rangeSliderDiagramWidget, SLOT( deleteSlot() ) );
		connect( rangeSliderDiagramWidget, SIGNAL( selectionRelesedSignal() ), this, SLOT( loadSelectionToSPMView() ) );

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
