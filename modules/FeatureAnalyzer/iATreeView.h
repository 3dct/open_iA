// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_TreeView.h"
#include "iABPMData.h"
#include "FeatureAnalyzerHelpers.h"    // for PorosityFilterID
#include "iAHMData.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <QWidget>
#include <QTableWidget>
#include <QModelIndex>
#include <QStringList>

class QContextMenuEvent;
class QMenu;

class QTableWidget;

struct iASelection;

typedef iAQTtoUIConnector<QWidget, Ui_treeView> TreeViewConnector;

class iATreeView : public TreeViewConnector
{
	Q_OBJECT

public:
	iATreeView(QWidget* parent = nullptr);
	virtual ~iATreeView();
	void SetData( QTableWidget * const data, const QMap<QString, double> * gtPorosityMap, int runsOffset );
	QList<QTreeWidgetItem *> * getLastSelectedItems();
	void setSelection( QList<QTreeWidgetItem *> * selItems );
	const QTableWidget * GetSPMData();

protected:
	QList<PorosityFilterID> getAlgorithmsInfo( const QList<QTreeWidgetItem *> & selectedItems ) const;
	QString getDatasetName( const QTreeWidgetItem * item ) const;
	QString getFilterName( const QTreeWidgetItem * item ) const;
	QList<QTreeWidgetItem *> aggregateRuns( const QList<QTreeWidgetItem *> & selectedItems ) const;
	void processItem( QTreeWidgetItem * item, QList<QTreeWidgetItem *> * outList ) const;
	QList<int> getSelectedIndices( const QList<QTreeWidgetItem *> & selectedItems );
	void processParameters( const QList<PorosityFilterID> & filterIds, QStringList & header, QStringList & outParamNames ) const;
	QStringList getAllDatasets( const QList<QTreeWidgetItem *> & items ) const;
	QStringList getAllFilterNames( const QList<QTreeWidgetItem *> & items ) const;
	QStringList getAllUniqueEntries( const QList<QTreeWidgetItem *> & items, int col ) const;
	virtual bool eventFilter( QObject *obj, QEvent *event );
	bool updateSelectedRunsData();
	bool calculatedSelectedRunsData( QList<QTreeWidgetItem*> selectedItems );
	bool updateSelectedPCData();
	bool updateSelectedBPMData();
	bool updateSelectedSSData(QList<QTreeWidgetItem*> items);
	bool updateSelectedHMData();
	void loadSelectionToRSD( QList<QTreeWidgetItem*> selectedItems );
	bool filteredItemsForSelection( QModelIndexList & selIds );

	void initFilteringGUI();
	QStringList getFilterDatasets();	//get lists of datasets to use for filtering
	QStringList getFilterPipelines();	//get lists of pipelines to use for filtering
	void updateFilteredItems( QStringList const & filtPipes, QStringList const & filtDsets );	//filter pipeline/dataset entries, store result in the list
	void loadFilteredItemsToWidget();	//add filtered items with grouping

signals:
	void loadSelectionToSPMSignal( const QTableWidget * );
	void loadSelectionToPCSignal( const QTableWidget * );
	void loadSelectionToPDMSignal( const iABPMData *, const iAHMData * );
	void loadSelectionToRSDSignal( const QTableWidget * );
	void loadSelectionToSSSignal( const QTableWidget *, QString selText);
	void loadSelectionsToSSSignal( const QList< QPair<QTableWidget *, QString> > * );
	void loadDatasetsToPreviewSignal( QStringList datasets );
	void loadAllDatasetsByIndicesSignal( QStringList sel_datasets, QList<int> indices );
	void selectionModified( QList<QTreeWidgetItem*>* );
	void clearOldRSDViewSignal();

public slots:
	void loadSelectionToSPM();
	void loadSelectionToSS();
	void loadSelectionToSS( iASelection * sel );
	void loadSelectionsToSS( QList<iASelection*>  sels );
	void loadSelectionToPC();
	void loadSelectionToPDM();

	void forwardRSDSelection();
	void loadFilteredDataToOverview();
	void loadOverviewSelectionToSPM( QModelIndexList indices );

protected slots:
	void adjustTreeWidgetColumnWidths();
	void filteringShow( bool show = false );
	void applyFilter();

protected:
	const QMap<QString, double> * m_gtPorosityMap;
	QMenu * m_contextMenu;
	QTableWidget * m_selectedRunsData;
	QTableWidget * m_selectedPCData;
	QTableWidget * m_selectedRSDData;
	QTableWidget * m_selectedSSData;
	QStringList m_datasets;
	QStringList m_selectedDatasets;
	QList<int> m_selDatasetsInds;
	QStringList m_pipelines;
	int m_runsOffset;
	iABPMData m_selectedBPMData;
	iAHMData m_selectedHMData;
	QMap < QPair<int, int>, int > m_treeIdToTableId;
	QTableWidget * m_dataExternal;
	QList<QTreeWidgetItem *> m_pipeDsetItems, m_runItems, m_topLevelItems, m_filteredPipeDsetItems;
	QList<QTreeWidgetItem *> m_lastSelectedItems;
	QStringList m_filteredDsets, m_filteredPipes;

	QTableWidget m_filteredSSData;
	QList< QPair<QTableWidget *, QString> > m_compareSSData;
};
