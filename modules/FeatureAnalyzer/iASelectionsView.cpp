// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASelectionsView.h"

iASelectionsView::iASelectionsView(QWidget* parent) :
	SelectionsConnector(parent),
	m_curSelection( new iASelection() ),
	m_selCounter( 1 )
{
	connect(tbCreate, &QToolButton::clicked, this, &iASelectionsView::saveCurrentSelection);
	connect(tbSwitch, &QToolButton::clicked, this, &iASelectionsView::loadSelection);
	connect(tbDelete, &QToolButton::clicked, this, &iASelectionsView::deleteSelection);
	connect(tbAddVis, &QToolButton::clicked, this, &iASelectionsView::visualizeSelectionSlot);
	connect(tbCompare, &QToolButton::clicked, this, &iASelectionsView::compareSelectionsSlot);
}

iASelectionsView::~iASelectionsView()
{
	for (int i = 0; i < m_selections.size(); ++i)
	{

		delete m_selections[i];
	}
}

void iASelectionsView::selectionModifiedTreeView( QList<QTreeWidgetItem*> * selItems )
{
	m_curSelection->setTreeItems( selItems );
}

void iASelectionsView::selectionModifiedSPMView( vtkVector2i curPlot, vtkIdTypeArray * selIds )
{
	m_curSelection->setIds( selIds );
	m_curSelection->spmCurPlot = curPlot;
}

void iASelectionsView::selectionModifiedPDMView( QModelIndexList selInds )
{
	m_curSelection->setPDMIndices( selInds );
}

void iASelectionsView::saveCurrentSelection()
{
	iASelection * savedSel = m_curSelection;
	m_selections.push_back( savedSel );
	m_curSelection = new iASelection( *savedSel );
	//add item to list widget
	listWidget->addItem( "Selection " + QString::number( m_selCounter++ ) );
	//make new item editable
	QListWidgetItem * item = listWidget->item( listWidget->count() - 1 );
	item->setFlags( item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable );
	item->setCheckState( Qt::Unchecked );
	m_selections.last()->selText = item->text();
}

void iASelectionsView::loadSelection()
{
	int row = listWidget->currentRow();
	if (row < 0)
	{
		return;
	}
	emit loadedSelection( m_selections[row] );
}

void iASelectionsView::deleteSelection()
{
	int row = listWidget->currentRow();
	if (row < 0)
	{
		return;
	}
	listWidget->takeItem( row );
	delete m_selections[row];
	m_selections.removeAt( row );
}

void iASelectionsView::visualizeSelectionSlot()
{
	int row = listWidget->currentRow();
	if (row < 0)
	{
		return;
	}
	QString selText = listWidget->item( row )->text();
	m_selections[row]->selText = selText;
	emit visualizeSelection( m_selections[row] );
}

void iASelectionsView::compareSelectionsSlot()
{
	QList<int> checkedRows;
	for( int i = 0; i < listWidget->count(); ++i )
	{
		if (listWidget->item(i)->checkState())
		{
			checkedRows.append(i);
		}
	}
	if (checkedRows.size() <= 1)
	{
		return;
	}
	QList<iASelection*> sels;
	for (int row: checkedRows)
	{
		QString selText = listWidget->item( row )->text();
		m_selections[row]->selText = selText;
		sels.push_back( m_selections[row] );
	}
	emit compareSelections( sels );
}
