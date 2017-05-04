/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "pch.h"
#include "dlg_fileEdit.h"
// iA
#include "iA4DCTSettings.h"
// Qt
#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>
#include <QString>

typedef std::pair<QString, QString> FileTypePair;
FileTypePair FileTypes[ ] = { std::make_pair( "regular file", "" ),
							  std::make_pair( "extracted fibers", S_4DCT_EXTRACTED_FIBERS ),
							  std::make_pair( "extracted curved fibers", S_4DCT_EXTRACTED_CURVED_FIBERS ),
							  std::make_pair( "extracted fiber image", S_4DCT_EXTRACTED_FIBER_IMAGE ) };

dlg_fileEdit::dlg_fileEdit( QWidget* parent /*= 0*/ ) :
	QDialog( parent )
{
	setupUi( this );
	connect( pbBrowse, SIGNAL( clicked( ) ), this, SLOT( onBrowseButtonClick( ) ) );
	connect( cbType, SIGNAL( currentIndexChanged( int ) ), this, SLOT( fileTypeChanged( int ) ) );

	int n = sizeof( FileTypes ) / sizeof( FileTypePair );
	for( int i = 0; i < n; i++ ) {
		cbType->addItem( FileTypes[i].first );
	}
}

dlg_fileEdit::~dlg_fileEdit( )
{ /* net implemented yet*/ }

void dlg_fileEdit::onBrowseButtonClick( )
{
	// open dialog
	QSettings settings;
	QString fileName = QFileDialog::getOpenFileName( this, "Open file", settings.value( S_4DCT_ADD_FILE_DIR ).toString( ) );
	QFileInfo fileInfo( fileName );
	if( !fileInfo.exists( ) ) {
		return;
	}
	settings.setValue( S_4DCT_ADD_FILE_DIR, fileInfo.dir( ).absolutePath( ) );

	lePath->setText( fileName );
	if( !leName->isReadOnly( ) ) {
		leName->setText( fileInfo.baseName( ) );
	}
}

QString dlg_fileEdit::getFilePath( )
{
	return lePath->text( );
}

QString dlg_fileEdit::getFileName( )
{
	return leName->text( );
}

void dlg_fileEdit::fileTypeChanged( int index )
{
	FileTypePair currentType = FileTypes[index];
	if( currentType.second.isEmpty( ) ) {
		leName->setReadOnly( false );
		leName->setText( "" );
	}
	else {
		leName->setReadOnly( true );
		leName->setText( currentType.second );
	}
}
