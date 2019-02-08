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
#include "iASelectionsView.h"

iASelectionsView::iASelectionsView( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	: SelectionsConnector( parent, f ),
	m_curSelection( new iASelection() ),
	m_selCounter( 1 )
{
	connect( tbCreate, SIGNAL( clicked() ), this, SLOT( saveCurrentSelection() ) );
	connect( tbSwitch, SIGNAL( clicked() ), this, SLOT( loadSelection() ) );
	connect( tbDelete, SIGNAL( clicked() ), this, SLOT( deleteSelection() ) );
	connect( tbAddVis, SIGNAL( clicked() ), this, SLOT( visualizeSelectionSlot() ) );
	connect( tbCompare, SIGNAL( clicked() ), this, SLOT( compareSelectionsSlot() ) );
}

iASelectionsView::~iASelectionsView()
{
	for( int i = 0; i < m_selections.size(); ++i )
		delete m_selections[i];
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
	if( row < 0 ) return;
	emit loadedSelection( m_selections[row] );
}

void iASelectionsView::deleteSelection()
{
	int row = listWidget->currentRow();
	if( row < 0 ) return;
	listWidget->takeItem( row );
	delete m_selections[row];
	m_selections.removeAt( row );
}

void iASelectionsView::visualizeSelectionSlot()
{
	int row = listWidget->currentRow();
	if( row < 0 ) return;
	QString selText = listWidget->item( row )->text();
	m_selections[row]->selText = selText;
	emit visualizeSelection( m_selections[row] );
}

void iASelectionsView::compareSelectionsSlot()
{
	QList<int> checkedRows;
	for( int i = 0; i < listWidget->count(); ++i )
	{
		if( listWidget->item( i )->checkState() )
			checkedRows.append( i ) ;
	}
	if( checkedRows.size() <= 1 )
		return;
	QList<iASelection*> sels;
	foreach( int row, checkedRows )
	{
		QString selText = listWidget->item( row )->text();
		m_selections[row]->selText = selText;
		sels.push_back( m_selections[row] );
	}
	emit compareSelections( sels );
}


