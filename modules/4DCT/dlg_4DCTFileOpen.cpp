// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_4DCTFileOpen.h"

#include <QString>

dlg_4DCTFileOpen::dlg_4DCTFileOpen( QWidget * parent )
	: QDialog( parent )
{
	setupUi( this );
	twFiles->setModel( &m_model );
	connect( twFiles, &QTreeView::doubleClicked, this, &dlg_4DCTFileOpen::onTreeViewDoubleClicked);
}

void dlg_4DCTFileOpen::setData( iA4DCTData * newData )
{
	m_data = newData;

	QStandardItem * rootNode = m_model.invisibleRootItem( );
	for( auto stageData : *m_data ) {
		QStandardItem * stageNode = new QStandardItem( QString::number( stageData->Force ) );
		for( auto file : stageData->Files ) {
			QStandardItem * fileNode = new QStandardItem( file.Name );
			fileNode->setEditable( false );
			stageNode->appendRow( fileNode );
		}
		rootNode->appendRow( stageNode );
	}
	twFiles->expandAll( );
}

void dlg_4DCTFileOpen::accept( )
{
	QItemSelectionModel * selectionModel = twFiles->selectionModel( );
	QModelIndexList selectedIndexes = selectionModel->selectedIndexes( );
	if( selectedIndexes.size( ) != 1 )
		return;
	QModelIndex index = selectedIndexes[0];
	setFileAndClose( index );
}

iA4DCTFileData dlg_4DCTFileOpen::getFile( )
{
	return m_file;
}

void dlg_4DCTFileOpen::onTreeViewDoubleClicked( const QModelIndex & index )
{
	setFileAndClose( index );
}

void dlg_4DCTFileOpen::setFileAndClose( const QModelIndex & index )
{
	int idx = index.parent( ).row( );
	if( idx < 0 )
		return;
	m_file = m_data->at( idx )->Files[index.row( )];
	QDialog::accept( );
}
