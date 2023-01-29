// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTListView.h"

#include "iAStageView.h"

#include <iAMainWindow.h>

#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QApplication>
#include <QStringListModel>

iA4DCTListView::iA4DCTListView( QWidget* parent/*=0*/ ) :
	QListView( parent )
{
	m_actOpen = new QAction( tr( "Open file" ), this );
	connect( m_actOpen, &QAction::triggered, this, &iA4DCTListView::openFile);
	m_actNew = new QAction( tr( "Add new file" ), this );
	connect( m_actNew, &QAction::triggered, this, &iA4DCTListView::addFile);

	m_menu = new QMenu( this );
	m_menu->addAction( m_actOpen );
	m_menu->addAction( m_actNew );
}

iA4DCTListView::~iA4DCTListView( )
{ }

void iA4DCTListView::setStageView( iAStageView * stageView )
{
	// assign local stage view variable
	m_stageView = stageView;
	updateData( );
}

void iA4DCTListView::updateData( )
{
	// fill the list view widget with data
	if( m_stageView->getData( ) == nullptr )
		return;
	m_data = &m_stageView->getData( )->Files;
	QStringListModel * model = new QStringListModel;
	QStringList list;
	for( auto fd : *m_data ) {
		list << fd.Name;
	}
	model->setStringList( list );
	setModel( model );
}

void iA4DCTListView::contextMenuEvent( QContextMenuEvent* event )
{
	m_menu->exec( event->globalPos( ) );
}

void iA4DCTListView::openFile( )
{
	QModelIndexList indexes = this->selectionModel( )->selectedIndexes( );
	if( indexes.size( ) > 1 )
	{
		return;
	}
	iAMainWindow* win = qobject_cast<iAMainWindow*>(QApplication::activeWindow());
	if( win)
	{
		win->loadFileNew( m_data->at( indexes[0].row( ) ).Path, nullptr);
	}
}

void iA4DCTListView::addFile( )
{
	if( m_stageView != nullptr )
		m_stageView->addFile( );
}
