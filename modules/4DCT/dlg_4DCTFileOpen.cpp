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
#include "dlg_4DCTFileOpen.h"

#include <QString>

dlg_4DCTFileOpen::dlg_4DCTFileOpen( QWidget * parent )
	: QDialog( parent )
{
	setupUi( this );
	twFiles->setModel( &m_model );
	connect( twFiles, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( onTreeViewDoubleClicked( QModelIndex ) ) );
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
