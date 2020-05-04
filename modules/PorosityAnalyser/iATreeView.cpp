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
#include "iATreeView.h"

#include "iACSVToQTableWidgetConverter.h"
#include "iASelection.h"

#include <iAConsole.h>

#include <vtkIdTypeArray.h>

#include <QAbstractItemView>
#include <QContextMenuEvent>
#include <QMenu>
#include <QTreeWidget>
#include <QPair>

iATreeView::iATreeView( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	: TreeViewConnector( parent, f ),
	m_contextMenu( new QMenu( this ) ),
	m_selectedRunsData( new QTableWidget() ),
	m_selectedPCData( new QTableWidget() ),
	m_selectedRSDData( new QTableWidget() ),
	m_selectedSSData( new QTableWidget() ),
	m_dataExternal( 0 )
{
	treeWidget->setSelectionMode( QAbstractItemView::ExtendedSelection );
	treeWidget->viewport()->installEventFilter( this );
	m_contextMenu->setStyleSheet( contextMenuStyle );
	m_contextMenu->addAction( "=> Scatter Plot Matrix", this, SLOT( loadSelectionToSPM() ) );
	m_contextMenu->addAction( "=> Parallel Coordinates", this, SLOT( loadSelectionToPC() ) );
	m_contextMenu->addAction( "=> Pipeline/Dataset Matrix", this, SLOT( loadSelectionToPDM() ) );
	m_contextMenu->addAction( "=> Range Slider Diagram", this, SLOT( forwardRSDSelection() ) );
	m_contextMenu->addAction( "=> Slice Segmentation View", this, SLOT( loadSelectionToSS() ) );
	connect(treeWidget, &QTreeWidget::itemCollapsed, this, &iATreeView::adjustTreeWidgetColumnWidths);
	connect(treeWidget, &QTreeWidget::itemExpanded, this, &iATreeView::adjustTreeWidgetColumnWidths);

	filterWidget->hide();
	lwFiltDataset->setSelectionMode( QAbstractItemView::ExtendedSelection );
	lwFiltPipeline->setSelectionMode( QAbstractItemView::ExtendedSelection );
	connect(tbFiltering, &QToolButton::clicked, this, &iATreeView::filteringShow);
	connect(cbFiltDataset, &QCheckBox::clicked, this, &iATreeView::applyFilter);
	connect(cbFiltPipeline, &QCheckBox::clicked, this, &iATreeView::applyFilter);
	connect(lwFiltDataset,  &QListWidget::itemChanged, this, &iATreeView::applyFilter);
	connect(lwFiltPipeline, &QListWidget::itemChanged, this, &iATreeView::applyFilter);
	connect(cbGrouping, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iATreeView::applyFilter);
}

iATreeView::~iATreeView()
{
	delete m_selectedPCData;
	delete m_selectedRunsData;
}

void iATreeView::SetData( QTableWidget * const dataTable, const QMap<QString, double> * gtPorosityMap, int runsOffset )
{
	m_dataExternal = dataTable;
	m_gtPorosityMap = gtPorosityMap;
	m_runsOffset = runsOffset;
	treeWidget->setColumnCount( m_dataExternal->columnCount() );

	QList<QString> listAlg_DS;
	QList<int> indicesAlg_DS;
	listAlg_DS << m_dataExternal->item( 0, groupColInd )->text();
	for( int i = 0; i < m_dataExternal->rowCount(); ++i )
	{
		int indexOf = listAlg_DS.indexOf( m_dataExternal->item( i, groupColInd )->text() );
		if( indexOf == -1 )
		{
			listAlg_DS << m_dataExternal->item( i, groupColInd )->text();
			indicesAlg_DS.push_back( listAlg_DS.size() - 1 );
		}
		else
		{
			indicesAlg_DS.push_back(indexOf);
		}
	}

	m_pipeDsetItems.clear();
	m_runItems.clear();
	m_topLevelItems.clear();

	for (int i = 0; i < listAlg_DS.size(); ++i)
	{
		m_pipeDsetItems.append(new QTreeWidgetItem(QStringList(listAlg_DS[i])));
	}

	m_treeIdToTableId.clear();
	int colCnt = m_dataExternal->columnCount();
	for( int i = 0; i < m_dataExternal->rowCount(); ++i )
	{
		QTreeWidgetItem * item = new QTreeWidgetItem;
		for( int c = 0; c < colCnt; ++c )
		{
			QTableWidgetItem * dataItem = m_dataExternal->item( i, c );
			if (dataItem)
			{
				item->setText(c, dataItem->text());
			}
		}
		m_pipeDsetItems[indicesAlg_DS[i]]->addChild( item );
		int ind = m_pipeDsetItems[indicesAlg_DS[i]]->indexOfChild( item );
		m_treeIdToTableId[ qMakePair( indicesAlg_DS[i], ind ) ] = i;
		m_runItems.push_back( item );
	}
	m_datasets = getAllDatasets( m_runItems );
	m_pipelines = getAllFilterNames( m_runItems );
	initFilteringGUI();
	applyFilter();
}

bool iATreeView::updateSelectedRunsData()
{
	return calculatedSelectedRunsData( treeWidget->selectedItems() );
}

bool iATreeView::calculatedSelectedRunsData( QList<QTreeWidgetItem*> selectedItems )
{
	m_lastSelectedItems = selectedItems;
	if (!m_lastSelectedItems.size())
	{
		return false;
	}

	//fill in header and numbers of parameters
	QStringList inParamNames;
	QStringList outParamNames;
	processParameters( getAlgorithmsInfo( m_lastSelectedItems ), inParamNames, outParamNames );
	int inParCnt = inParamNames.size(), outParCnt = outParamNames.size();

	//aggregate all runs
	QList<QTreeWidgetItem *> finalItems = aggregateRuns( m_lastSelectedItems );

	//Debug
	//for (int i = 0; i < finalItems.size(); ++i)
	//{
	//	QString s;
	//	for (int j = 0; j < finalItems[i]->columnCount(); ++j)
	//	{
	//		s.append(finalItems[i]->text(j));
	//		if (j < finalItems[i]->columnCount() - 1);
	//		s.append(",");
	//	}
	//	DEBUG_LOG(QString(s));
	//}

	//insert header
	m_selectedRunsData->clear();
	m_selectedRunsData->setColumnCount( inParCnt + outputSPMParamNum + outParCnt + 2 + 3 + 6 + 1);//+deviation from rererence+dataset index +3dice errors + 6avg feature outputs + mask path
	m_selectedRunsData->setRowCount( 1 );
	int headerOffset = 0;
	for (int i = 0; i < inParCnt; ++i)
	{
		m_selectedRunsData->setItem(0, headerOffset++, new QTableWidgetItem(inParamNames[i]));
	}
	m_selectedRunsData->setItem( 0, headerOffset++, new QTableWidgetItem( "Elapsed Time" ) );
	m_selectedRunsData->setItem( 0, headerOffset++, new QTableWidgetItem( "Porosity" ) );
	m_selectedRunsData->setItem( 0, headerOffset++, new QTableWidgetItem( "Deviat. from Ref." ) );
	m_selectedRunsData->setItem( 0, headerOffset++, new QTableWidgetItem( "Dataset Index" ) );
	m_selectedRunsData->setItem( 0, headerOffset++, new QTableWidgetItem( "False Positive Error" ) );
	m_selectedRunsData->setItem( 0, headerOffset++, new QTableWidgetItem( "False Negative Error" ) );
	m_selectedRunsData->setItem( 0, headerOffset++, new QTableWidgetItem( "Dice" ) );
	m_selectedRunsData->setItem(0, headerOffset++, new QTableWidgetItem("FeatureCnt"));
	m_selectedRunsData->setItem(0, headerOffset++, new QTableWidgetItem("AvgFeatureVol"));
	m_selectedRunsData->setItem(0, headerOffset++, new QTableWidgetItem("AvgFeaturePhi"));
	m_selectedRunsData->setItem(0, headerOffset++, new QTableWidgetItem("AvgFeatureTheta"));
	m_selectedRunsData->setItem(0, headerOffset++, new QTableWidgetItem("AvgFeatureRoundness"));
	m_selectedRunsData->setItem(0, headerOffset++, new QTableWidgetItem("AvgFeatureLength"));

	for (int i = 0; i < outParCnt; ++i)
	{
		m_selectedRunsData->setItem(0, headerOffset++, new QTableWidgetItem(outParamNames[i]));
	}
	m_selectedRunsData->setItem( 0, headerOffset++, new QTableWidgetItem( "Mask Path" ) );

	//go through items and parse them
	m_selectedDatasets.clear();
	m_selDatasetsInds.clear();
	for (const QTreeWidgetItem * item: finalItems)
	{
		double porosity, devRefPorosity;
		QString datasetName = getDatasetName( item );
		if( !m_selectedDatasets.contains( datasetName ) )
		{
			m_selectedDatasets.push_back( datasetName );
			m_selDatasetsInds.push_back( m_datasets.indexOf( datasetName ) );
		}
		int paramOffset = 0;
		int lastRow = m_selectedRunsData->rowCount();
		m_selectedRunsData->insertRow( lastRow );
		//parse input parameter values
		for( int i = 0; i < inParCnt; ++i )
		{
			QString s = item->text( m_runsOffset + paramsOffsetInRunsCSV + i);
			m_selectedRunsData->setItem( lastRow, paramOffset++, new QTableWidgetItem( item->text( m_runsOffset + paramsOffsetInRunsCSV + i ) ) );
		}
		//add runtime and porosity
		int itemOffset = m_runsOffset + 1;
		m_selectedRunsData->setItem( lastRow, paramOffset++, new QTableWidgetItem( item->text( itemOffset++ ) ) );
		porosity = item->text( itemOffset ).toDouble();
		m_selectedRunsData->setItem( lastRow, paramOffset++, new QTableWidgetItem( item->text( itemOffset++ ) ) );
		//insert deviation from the reference porosity
		devRefPorosity = porosity - (*m_gtPorosityMap)[datasetName];
		m_selectedRunsData->setItem( lastRow, paramOffset++, new QTableWidgetItem( QString::number( devRefPorosity ) ) );
		//insert dataset index
		m_selectedRunsData->setItem( lastRow, paramOffset++, new QTableWidgetItem( QString::number( m_datasets.indexOf( datasetName ) ) ) );
		//insert dice errors
		int errorInd = m_runsOffset + paramsOffsetInRunsCSV - 9;
		//Debug
		/*QString fpe = item->text( errorInd );
		QString fne = item->text( errorInd + 1 );
		QString dice = item->text( errorInd + 2 );*/
		m_selectedRunsData->setItem( lastRow, paramOffset++, new QTableWidgetItem( item->text( errorInd++ ) ) );
		m_selectedRunsData->setItem( lastRow, paramOffset++, new QTableWidgetItem( item->text( errorInd++) ) );
		m_selectedRunsData->setItem( lastRow, paramOffset++, new QTableWidgetItem( item->text( errorInd++ ) ) );

		//Parse average feature outputs (FeatureCount, AvgFeatureVol, AvgFeaturePhi, AvgFeatureTheta, AvgFeatureRoundness, AvgFeatureLength)
		m_selectedRunsData->setItem(lastRow, paramOffset++, new QTableWidgetItem(item->text(errorInd++)));
		m_selectedRunsData->setItem(lastRow, paramOffset++, new QTableWidgetItem(item->text(errorInd++)));
		m_selectedRunsData->setItem(lastRow, paramOffset++, new QTableWidgetItem(item->text(errorInd++)));
		m_selectedRunsData->setItem(lastRow, paramOffset++, new QTableWidgetItem(item->text(errorInd++)));
		m_selectedRunsData->setItem(lastRow, paramOffset++, new QTableWidgetItem(item->text(errorInd++)));
		m_selectedRunsData->setItem(lastRow, paramOffset++, new QTableWidgetItem(item->text(errorInd)));

		//parse output parameter values
		for (int i = 0; i < outParCnt; ++i)
		{
			m_selectedRunsData->setItem(lastRow, paramOffset++, new QTableWidgetItem(item->text(itemOffset++)));
		}
		//mask path
		int maskPathInd =  m_runsOffset + paramsOffsetInRunsCSV - 10;
		m_selectedRunsData->setItem( lastRow, paramOffset, new QTableWidgetItem( item->text( maskPathInd ) ) );
	}
	return true;
}

bool iATreeView::updateSelectedSSData( QList<QTreeWidgetItem*> items )
{
	if (!items.size())
	{
		return false;
	}

	//aggregate all runs
	QList<QTreeWidgetItem *> finalItems = aggregateRuns( items );

	//insert header
	m_selectedSSData->clear();
	int colCnt = finalItems.first()->columnCount();
	m_selectedSSData->setColumnCount( colCnt );
	m_selectedSSData->setRowCount( 0 );

	//go through items and parse them
	for (const QTreeWidgetItem * item: finalItems)
	{
		int lastRow = m_selectedSSData->rowCount();
		m_selectedSSData->insertRow( lastRow );
		//parse input parameter values
		for (int i = 0; i < colCnt; ++i)
		{
			m_selectedSSData->setItem(lastRow, i, new QTableWidgetItem(item->text(i)));
		}
	}
	return true;
}

void iATreeView::loadSelectionToSPM()
{
	if (!updateSelectedRunsData())
	{
		return;
	}
	emit loadSelectionToSPMSignal( m_selectedRunsData );
	emit loadDatasetsToPreviewSignal( m_selectedDatasets );
	emit loadAllDatasetsByIndicesSignal( m_selectedDatasets, m_selDatasetsInds );
	emit selectionModified( &m_lastSelectedItems );
}

const QTableWidget * iATreeView::GetSPMData()
{
	if (!updateSelectedRunsData())
	{
		return nullptr;
	}
	return m_selectedRunsData;
}

void iATreeView::loadSelectionToSS()
{
	// selection from raw tree widget
	if (!updateSelectedSSData(treeWidget->selectedItems()))
	{
		return;
	}
	emit loadSelectionToSSSignal( m_selectedSSData, "" );
	emit selectionModified( &m_lastSelectedItems );
}

void iATreeView::loadSelectionToSS( iASelection * sel )
{
	if (!filteredItemsForSelection(sel->pdmSelection))
	{
		return;
	}
	if (!updateSelectedSSData(m_filteredPipeDsetItems))
	{
		return;
	}
	//filter the data using selection from spm
	vtkIdType size = sel->ids->GetSize();
	m_filteredSSData.clearContents();
	int colCnt = m_selectedSSData->columnCount();
	m_filteredSSData.setColumnCount( colCnt );
	m_filteredSSData.setRowCount( 0 );
	for( vtkIdType i = 0; i < size; ++i )
	{
		int lastRow = m_filteredSSData.rowCount();
		m_filteredSSData.insertRow( lastRow );
		for( int j = 0; j < colCnt; ++j )
		{
			int ind = (int)sel->ids->GetValue( i );
			m_filteredSSData.setItem( lastRow, j, new QTableWidgetItem( *m_selectedSSData->item( ind, j ) ) );
		}
	}
	emit loadSelectionToSSSignal( &m_filteredSSData, sel->selText );
}


void iATreeView::loadSelectionsToSS( QList<iASelection*> sels )
{
	for (QPair<QTableWidget *, QString> pair: m_compareSSData)
	{
		delete pair.first;
	}
	m_compareSSData.clear();

	for (iASelection * s: sels)
	{
		if (!filteredItemsForSelection(s->pdmSelection))
		{
			return;
		}
		if (!updateSelectedSSData(m_filteredPipeDsetItems))
		{
			return;
		}
		//filter the data using selection from spm
		int colCnt = m_selectedSSData->columnCount();
		QTableWidget * tw = new QTableWidget( 0, colCnt );
		vtkIdType size = s->ids->GetSize();
		for( vtkIdType i = 0; i < size; ++i )
		{
			int lastRow = tw->rowCount();
			tw->insertRow( lastRow );
			for( int j = 0; j < colCnt; ++j )
			{
				int ind = (int)s->ids->GetValue( i );
				//QString text = m_selectedSSData->item( ind, j )->text();
				tw->setItem( lastRow, j, new QTableWidgetItem( *m_selectedSSData->item( ind, j ) ) );
			}
		}
		m_compareSSData.push_back( qMakePair( tw, s->selText ) );
	}
	emit loadSelectionsToSSSignal( &m_compareSSData );
}


bool iATreeView::updateSelectedPCData()
{
	m_lastSelectedItems = treeWidget->selectedItems();
	if (!m_lastSelectedItems.size())
	{
		return false;
	}

	//aggregate all runs
	QList<QTreeWidgetItem *> finalItems = aggregateRuns( m_lastSelectedItems );

	//get list of datasets
	QStringList datasets = getAllDatasets( finalItems );

	m_selectedPCData->clear();
	m_selectedPCData->setColumnCount( datasets.size() );
	m_selectedPCData->setRowCount( 1 );

	QList<int> offsets;
	for( int i = 0; i < datasets.size(); ++i )
	{
		m_selectedPCData->setItem( 0, i, new QTableWidgetItem( datasets[i] ) );
		offsets.push_back( 1 ); //because of header
	}
	//go through items and parse them
	for (const QTreeWidgetItem * item: finalItems)
	{
		int column = datasets.indexOf( item->text( datasetColInd ) );
		int offset = offsets[column];
		if( offset == m_selectedPCData->rowCount() )
		{
			int lastRow = m_selectedPCData->rowCount();
			m_selectedPCData->insertRow( lastRow );
		}
		m_selectedPCData->setItem( offset, column, new QTableWidgetItem( item->text( m_runsOffset + 2 ) ) );
		offsets[column] = offset + 1;
	}
	return true;
}

void iATreeView::loadSelectionToPC()
{
	if (!updateSelectedPCData())
	{
		return;
	}
	emit loadSelectionToPCSignal( m_selectedPCData );
	emit selectionModified( &m_lastSelectedItems );
}

bool iATreeView::updateSelectedBPMData()
{
	m_selectedBPMData.clear();
	//aggregate all runs
	m_lastSelectedItems = treeWidget->selectedItems();
	if (!m_lastSelectedItems.size())
	{
		return false;
	}
	QList<QTreeWidgetItem *> finalItems = aggregateRuns( m_lastSelectedItems );

	//prepare structures
	m_selectedBPMData.datasets = getAllDatasets( finalItems );
	m_selectedBPMData.filters = getAllFilterNames( finalItems );

	for( int i = 0; i < m_selectedBPMData.filters.size(); ++i )
	{
		QList< iABoxPlotData > sublist;
		for (int j = 0; j < m_selectedBPMData.datasets.size(); ++j)
		{
			sublist.push_back(iABoxPlotData());
		}
		m_selectedBPMData.boxPlots.push_back( sublist );
	}

	QList< QList< QVector<double> > > bpData;
	for( int i = 0; i < m_selectedBPMData.filters.size(); ++i )
	{
		QList< QVector<double> > sublist;
		for (int j = 0; j < m_selectedBPMData.datasets.size(); ++j)
		{
			sublist.push_back(QVector<double>());
		}
		bpData.push_back( sublist );
	}

	//go through items and fetch porosities
	for (const QTreeWidgetItem* item : finalItems)
	{
		int dsInd = m_selectedBPMData.datasets.indexOf( getDatasetName( item ) );
		int filtInd = m_selectedBPMData.filters.indexOf( getFilterName( item ) );

		double porosity = item->text( m_runsOffset + 2 ).toDouble();
		bpData[filtInd][dsInd].push_back( porosity );
	}

	//calculate box plots
	for (int i = 0; i < m_selectedBPMData.filters.size(); ++i)
	{
		for (int j = 0; j < m_selectedBPMData.datasets.size(); ++j)
		{
			m_selectedBPMData.boxPlots[i][j].CalculateBoxPlot(bpData[i][j].data(), bpData[i][j].size(), true);
		}
	}

	return true;
}

void iATreeView::forwardRSDSelection()
{
	loadSelectionToRSD( m_filteredPipeDsetItems );
}

void iATreeView::loadSelectionToRSD( QList<QTreeWidgetItem*> selectedItems )
{
	m_lastSelectedItems = selectedItems;

	if (selectedItems.isEmpty())
	{
		emit clearOldRSDViewSignal();
	}
	if (!m_lastSelectedItems.size())
	{
		return;
	}
	//Currently only one algo + one dataset
	if ( m_lastSelectedItems.size() > 1 )
	{
		emit displayMessage( "'Parameter Range Slider' only can show one pipeline/dataset combination at the same time." );
		emit clearOldRSDViewSignal();
		return;
	}

	//get algorithms info
	QList<PorosityFilterID> filterIds = getAlgorithmsInfo( m_lastSelectedItems );

	//fill in header and numbers of parameters
	QStringList header;
	QStringList outParamNames;
	processParameters( filterIds, header, outParamNames );

	//Parameter-free algorithms in Parameter Range Slider View makes no sense
	if ( header.size() < 1 )
	{
		emit displayMessage( "A parameter-free algorithm can not be shown in the Parameter Range Slider View." );
		return;
	}

	//aggregate all runs
	QList<QTreeWidgetItem *> finalItems = aggregateRuns( m_lastSelectedItems );

	//insert header
	m_selectedRSDData->clear();
	m_selectedRSDData->setColumnCount( header.size() + 4 );
	m_selectedRSDData->setRowCount( 1 );
	for (int i = 0; i < header.size(); ++i)
	{
		m_selectedRSDData->setItem(0, i, new QTableWidgetItem(header[i]));
	}
	int headerOffset = header.size();
	m_selectedRSDData->setItem( 0, headerOffset++, new QTableWidgetItem( "Porosity" ) );
	m_selectedRSDData->setItem( 0, headerOffset++, new QTableWidgetItem( "Deviation" ) );
	m_selectedRSDData->setItem( 0, headerOffset++, new QTableWidgetItem( "Dataset" ) );
	m_selectedRSDData->setItem( 0, headerOffset++, new QTableWidgetItem( "Algorithm" ) );

	//go through items and parse them
	for (const QTreeWidgetItem* item : finalItems)
	{
		double porosity, devRefPorosity;
		QString datasetName = getDatasetName( item );
		QString filterName = getFilterName( item );
		int paramOffset = 0;
		int lastRow = m_selectedRSDData->rowCount();
		m_selectedRSDData->insertRow( lastRow );
		//parse input parameter values
		for (int i = 0; i < header.size(); ++i)
		{
			m_selectedRSDData->setItem(lastRow, paramOffset++, new QTableWidgetItem(item->text(m_runsOffset + paramsOffsetInRunsCSV + i)));
		}
		//insert porosity
		int itemOffset = m_runsOffset + 2;
		porosity = item->text( itemOffset ).toDouble();
		m_selectedRSDData->setItem( lastRow, paramOffset++, new QTableWidgetItem( item->text( itemOffset++ ) ) );
		//insert deviation from the reference porosity
		devRefPorosity = porosity - ( *m_gtPorosityMap )[datasetName];
		m_selectedRSDData->setItem( lastRow, paramOffset++, new QTableWidgetItem( QString::number( devRefPorosity ) ) );
		//insert dataset name
		m_selectedRSDData->setItem( lastRow, paramOffset++, new QTableWidgetItem( datasetName ) );
		//insert algorithm name
		m_selectedRSDData->setItem( lastRow, paramOffset++, new QTableWidgetItem( filterName) );
	}
	emit loadSelectionToRSDSignal( m_selectedRSDData );
	emit selectionModified( &m_lastSelectedItems );
}

bool iATreeView::updateSelectedHMData()
{
	m_selectedHMData.clear();
	//aggregate all runs
	m_lastSelectedItems = treeWidget->selectedItems();
	if (!m_lastSelectedItems.size())
	{
		return false;
	}
	QList<QTreeWidgetItem *> finalItems = aggregateRuns( m_lastSelectedItems );

	//prepare structures
	m_selectedHMData.datasets = getAllDatasets( finalItems );
	m_selectedHMData.filters = getAllFilterNames( finalItems );

	for ( int i = 0; i < m_selectedHMData.datasets.size(); ++i )
	{
		QString datasetName = m_selectedHMData.datasets.at( i );
		double gtPorosity = ( *m_gtPorosityMap )[m_selectedHMData.datasets.at( i )];

		m_selectedHMData.gtPorosityMap.insertMulti( datasetName, gtPorosity );
	}

	for ( int i = 0; i < m_selectedHMData.filters.size(); ++i )
	{
		QList< iAHistogramPlotData > sublist;
		for (int j = 0; j < m_selectedHMData.datasets.size(); ++j)
		{
			sublist.push_back(iAHistogramPlotData());
		}
		m_selectedHMData.histogramPlots.push_back( sublist );
	}

	QList< QList< QVector<double> > > hpData;
	for ( int i = 0; i < m_selectedHMData.filters.size(); ++i )
	{
		QList< QVector<double> > sublist;
		for (int j = 0; j < m_selectedHMData.datasets.size(); ++j)
		{
			sublist.push_back(QVector<double>());
		}
		hpData.push_back( sublist );
	}

	//go through items and fetch porosities
	for (const QTreeWidgetItem* item : finalItems)
	{
		int dsInd = m_selectedHMData.datasets.indexOf( getDatasetName( item ) );
		int filtInd = m_selectedHMData.filters.indexOf( getFilterName( item ) );

		double porosity = item->text( m_runsOffset + 2 ).toDouble();
		hpData[filtInd][dsInd].push_back( porosity );
	}

	//calculate box plots
	for (int i = 0; i < m_selectedHMData.filters.size(); ++i)
	{
		for (int j = 0; j < m_selectedHMData.datasets.size(); ++j)
		{
			m_selectedHMData.histogramPlots[i][j].CalculateHistogramPlot(hpData[i][j].data(), hpData[i][j].size());
		}
	}

	return true;
}

bool iATreeView::eventFilter( QObject *obj, QEvent *event )
{
	if( event->type() == QEvent::MouseButtonRelease )
	{
		QMouseEvent * me = (QMouseEvent*)event;
		if (me->button() == Qt::RightButton)
		{
			m_contextMenu->exec(me->globalPos());
		}
	}

	return TreeViewConnector::eventFilter( obj, event );
}

QList<PorosityFilterID> iATreeView::getAlgorithmsInfo( const QList<QTreeWidgetItem *> & selectedItems ) const
{
	QTreeWidgetItem * firstItem = selectedItems[0];
	if (selectedItems[0]->childCount())
	{
		firstItem = selectedItems[0]->child(0);
	}
	QString algName = firstItem->text( algNameColInd );
	QList<PorosityFilterID> filterIds = parseFiltersFromString( algName );
	return filterIds;
}

QString iATreeView::getDatasetName( const QTreeWidgetItem * item ) const
{
	return item->text( datasetColInd );
}

QString iATreeView::getFilterName( const QTreeWidgetItem * item ) const
{
	return item->text( algNameColInd );
}

QList<QTreeWidgetItem *> iATreeView::aggregateRuns( const QList<QTreeWidgetItem *> & selectedItems ) const
{
	QList<QTreeWidgetItem *> finalItems;
	for (QTreeWidgetItem* item : selectedItems)
	{
		processItem(item, &finalItems);
	}
	return finalItems;
}

void iATreeView::processItem( QTreeWidgetItem * item, QList<QTreeWidgetItem *> * outList ) const
{
	if( !item->childCount() )
	{
		outList->append( item );
		return;
	}
	for (int i = 0; i < item->childCount(); ++i)
	{
		processItem(item->child(i), outList);
	}
}

QList<int> iATreeView::getSelectedIndices( const QList<QTreeWidgetItem *> & selectedItems )
{
	QList<int> selectedIndices;
	QList<QTreeWidgetItem *> finalItems = aggregateRuns( selectedItems );
	for (QTreeWidgetItem * item: finalItems)
	{
		QTreeWidgetItem * parentItem = item->parent();
		QPair<int, int> treeInds = qMakePair
		(
			treeWidget->indexOfTopLevelItem( parentItem ),
			parentItem->indexOfChild( const_cast<QTreeWidgetItem*>(item) )
		);
		selectedIndices.push_back( m_treeIdToTableId[treeInds] );
	}
	return selectedIndices;
}

void iATreeView::processParameters( const QList<PorosityFilterID> & filterIds, QStringList & header, QStringList & outParamNames ) const
{
	for (const PorosityFilterID fid: filterIds)
	{
		QList<ParamNameType> params = FilterIdToParamList[fid];
		for (const ParamNameType pnt: params)
		{
			header << pnt.name();
		}

		QList<ParamNameType> outParams = FilterIdToOutputParamList[fid];
		for (const ParamNameType pnt: outParams)
		{
			outParamNames << pnt.name();
		}
	}
}

void iATreeView::adjustTreeWidgetColumnWidths()
{
	bool wasEnabled = treeWidget->updatesEnabled();
	treeWidget->setUpdatesEnabled( false );
	for (int i = 0; i < treeWidget->columnCount(); i++)
	{
		treeWidget->resizeColumnToContents(i);
	}
	treeWidget->setUpdatesEnabled( wasEnabled );
}

QStringList iATreeView::getAllUniqueEntries( const QList<QTreeWidgetItem *> & items, int col ) const
{
	QStringList res;
	res << items[0]->text( col );
	for( const QTreeWidgetItem * item: items)
	{
		if (res.contains(item->text(col)))
		{
			continue;
		}
		res << item->text( col );
	}
	return res;
}

QStringList iATreeView::getAllDatasets( const QList<QTreeWidgetItem *> & items ) const
{
	return getAllUniqueEntries( items, datasetColInd );
}

QStringList iATreeView::getAllFilterNames( const QList<QTreeWidgetItem *> & items ) const
{
	return getAllUniqueEntries( items, algNameColInd );
}

void iATreeView::filteringShow( bool show /*= false */ )
{
	if (show)
	{
		filterWidget->show();
	}
	else
	{
		filterWidget->hide();
	}
}

void iATreeView::initFilteringGUI()
{
	for (QString d: m_datasets)
	{
		QListWidgetItem * i = new QListWidgetItem( d, lwFiltDataset );
		i->setFlags( i->flags() | Qt::ItemIsUserCheckable );
		i->setCheckState( Qt::Unchecked );
	}
	for (QString p: m_pipelines)
	{
		QListWidgetItem * i = new QListWidgetItem( p.replace( "_", " => " ), lwFiltPipeline );
		i->setFlags( i->flags() | Qt::ItemIsUserCheckable );
		i->setCheckState( Qt::Unchecked );
	}
}

void iATreeView::applyFilter()
{
	QObject* obj = sender();
	if (obj == lwFiltDataset && !cbFiltDataset->isChecked())
	{
		return;
	}
	if (obj == lwFiltPipeline && !cbFiltPipeline->isChecked())
	{
		return;
	}
	//get lists of datasets and pipelines to use for filtering
	updateFilteredItems( getFilterPipelines(), getFilterDatasets() );
	//add filtered items with grouping
	loadFilteredItemsToWidget();
	loadFilteredDataToOverview();
}

QList<QTreeWidgetItem *> * iATreeView::getLastSelectedItems()
{
	return &m_lastSelectedItems;
}

void iATreeView::setSelection( QList<QTreeWidgetItem *> * selItems )
{
	treeWidget->clearSelection();
	treeWidget->setUpdatesEnabled( false );
	for (QTreeWidgetItem* i : *selItems)
	{
		i->setSelected(true);
	}
	treeWidget->setUpdatesEnabled( true );
	treeWidget->update();
}

void iATreeView::loadFilteredDataToOverview()
{
	treeWidget->selectAll();
	loadSelectionToPDM();
	setSelection( &m_lastSelectedItems );
}

void iATreeView::loadSelectionToPDM()
{
	updateSelectedBPMData();
	updateSelectedHMData();
	emit loadSelectionToPDMSignal( &m_selectedBPMData, &m_selectedHMData );
	emit selectionModified( &m_lastSelectedItems );
}

QStringList iATreeView::getFilterDatasets()
{
	QStringList filtDsets;
	QList<QListWidgetItem *> selItems;
	if( cbFiltDataset->isChecked() )
	{
		for( int i = 0; i < lwFiltDataset->count(); ++i )
		{
			if (lwFiltDataset->item(i)->checkState())
			{
				selItems.append(lwFiltDataset->item(i));
			}
		}
	}
	else
	{
		selItems.clear();
		for (int i = 0; i < lwFiltDataset->count(); ++i)
		{
			selItems.append(lwFiltDataset->item(i));
		}
	}
	for (QListWidgetItem* i : selItems)
	{
		filtDsets << i->text();
	}
	return filtDsets;
}

QStringList iATreeView::getFilterPipelines()
{
	QStringList filtPipes;
	QList<QListWidgetItem *> selItems;
	if( cbFiltPipeline->isChecked() )
	{
		for( int i = 0; i < lwFiltPipeline->count(); ++i )
		{
			if (lwFiltPipeline->item(i)->checkState())
			{
				selItems.append(lwFiltPipeline->item(i));
			}
		}
	}
	else
	{
		selItems.clear();
		for (int i = 0; i < lwFiltPipeline->count(); ++i)
		{
			selItems.append(lwFiltPipeline->item(i));
		}
	}
	for (QListWidgetItem* i : selItems)
	{
		filtPipes << i->text();
	}
	return filtPipes;
}

void iATreeView::updateFilteredItems( QStringList const & filtPipes, QStringList const & filtDsets )
{
	m_filteredPipeDsetItems.clear();
	m_filteredDsets.clear();
	m_filteredPipes.clear();
	for( QTreeWidgetItem * i: m_pipeDsetItems )
	{
		QTreeWidgetItem * fi = i->child( 0 );
		QString d = getDatasetName( fi ), p = getFilterName( fi ).replace( "_", " => " );
		if (!filtPipes.contains(p))
		{
			continue;
		}
		if (!filtDsets.contains(d))
		{
			continue;
		}
		if (!m_filteredDsets.contains(d))
		{
			m_filteredDsets.push_back(d);
		}
		if (!m_filteredPipes.contains(p))
		{
			m_filteredPipes.push_back(p);
		}
		m_filteredPipeDsetItems.push_back( i );
	}
}

void iATreeView::loadFilteredItemsToWidget()
{
	treeWidget->setUpdatesEnabled( false );
	m_topLevelItems.clear();
	if( cbGrouping->currentIndex() == 0 ) //datasets
	{
		for (QString d: m_filteredDsets)
		{
			QTreeWidgetItem * newItem = new QTreeWidgetItem( QStringList( d ) );
			for (QTreeWidgetItem* i : m_filteredPipeDsetItems)
			{
				if (getDatasetName(i->child(0)) == d)
				{
					QTreeWidgetItem* childItem = i->clone();
					childItem->setText(0, getFilterName(i->child(0)).replace("_", " => "));
					newItem->addChild(childItem);
				}
			}
			m_topLevelItems.append( newItem );
		}
	}
	else if( cbGrouping->currentIndex() == 1 ) //pipelines
	{
		for (QString p: m_filteredPipes)
		{
			QTreeWidgetItem * newItem = new QTreeWidgetItem( QStringList( p ) );
			for (QTreeWidgetItem* i : m_filteredPipeDsetItems)
			{
				if (getFilterName(i->child(0)).replace("_", " => ") == p)
				{
					QTreeWidgetItem* childItem = i->clone();
					childItem->setText(0, getDatasetName(i->child(0)));
					newItem->addChild(childItem);
				}
			}
			m_topLevelItems.append( newItem );
		}
	}

	treeWidget->clear();
	treeWidget->insertTopLevelItems( 0, m_topLevelItems );
	adjustTreeWidgetColumnWidths();
	treeWidget->setUpdatesEnabled( true );
	treeWidget->update();
}

void iATreeView::loadOverviewSelectionToSPM( QModelIndexList indices )
{
	m_selectedRunsData->clear();
	m_selectedRunsData->setRowCount( 0 );
	m_selectedRunsData->setColumnCount( 0 );
	m_selectedDatasets.clear();
	m_selDatasetsInds.clear();
	filteredItemsForSelection( indices );
	loadSelectionToRSD( m_filteredPipeDsetItems );
	emit loadSelectionToSPMSignal( m_selectedRunsData );
	emit loadDatasetsToPreviewSignal( m_selectedDatasets );
	emit loadAllDatasetsByIndicesSignal( m_selectedDatasets, m_selDatasetsInds );
}

bool iATreeView::filteredItemsForSelection( QModelIndexList & selIds )
{
	if (!treeWidget->selectedItems().size())
	{
		return false;
	}
	QList<QTreeWidgetItem *> finalItems = aggregateRuns( treeWidget->selectedItems() );
	QStringList datasets = getAllDatasets( finalItems );
	QStringList filters = getAllFilterNames( finalItems );
	QStringList filtDsets, filtPipes;
	for (QModelIndex i : selIds)
	{
		int row = i.row() - 1, col = i.column() - 1; //-1 because of headers
		if (row < 0 || col < 0)
		{
			continue;
		}
		QString d = datasets[row];
		QString p = filters[col];
		if (!filtDsets.contains(d))
		{
			filtDsets.push_back(d);
		}
		if (!filtPipes.contains(p))
		{
			filtPipes.push_back(p);
		}
	}

	//updateFilteredItems( filtPipes, filtDsets ); <-pipeline selection error
	//Below: pipeline selection error fix, TODO: refactoring
	m_filteredPipeDsetItems.clear();
	m_filteredDsets.clear();
	m_filteredPipes.clear();
	for (QTreeWidgetItem * i : m_pipeDsetItems)
	{
		QTreeWidgetItem * fi = i->child( 0 );
		QString d = getDatasetName( fi ), p = getFilterName( fi );
		if (!filtPipes.contains(p))
		{
			continue;
		}
		if (!filtDsets.contains(d))
		{
			continue;
		}
		if (!m_filteredDsets.contains(d))
		{
			m_filteredDsets.push_back(d);
		}
		if (!m_filteredPipes.contains(p))
		{
			m_filteredPipes.push_back(p);
		}
		m_filteredPipeDsetItems.push_back( i );
	}

	calculatedSelectedRunsData( m_filteredPipeDsetItems );
	return true;
}
