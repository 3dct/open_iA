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
#include "iA4DCTListView.h"

#include "iA4DCTSettings.h"
#include "iAStageView.h"
#include "mainwindow.h"

#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QApplication>
#include <QStringListModel>

iA4DCTListView::iA4DCTListView( QWidget* parent/*=0*/ ) :
	QListView( parent )
{
	m_actOpen = new QAction( tr( "Open file" ), this );
	connect( m_actOpen, SIGNAL( triggered( ) ), this, SLOT( openFile( ) ) );
	m_actNew = new QAction( tr( "Add new file" ), this );
	connect( m_actNew, SIGNAL( triggered( ) ), this, SLOT( addFile( ) ) );

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
	MainWindow* win = qobject_cast<MainWindow*>( QApplication::activeWindow( ) );

	QModelIndexList indexes = this->selectionModel( )->selectedIndexes( );
	if( indexes.size( ) > 1 ) {
		return;
	}

	if( win != nullptr ) {
		win->loadFile( m_data->at( indexes[0].row( ) ).Path, false );
	}
}

void iA4DCTListView::addFile( )
{
	if( m_stageView != nullptr )
		m_stageView->addFile( );
}
