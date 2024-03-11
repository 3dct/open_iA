// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_fileEdit.h"

#include "iA4DCTSettings.h"

#include <QSettings>
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
	connect( pbBrowse, &QPushButton::clicked, this, &dlg_fileEdit::onBrowseButtonClick);
	connect( cbType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_fileEdit::fileTypeChanged);

	int n = sizeof( FileTypes ) / sizeof( FileTypePair );
	for( int i = 0; i < n; i++ )
	{
		cbType->addItem( FileTypes[i].first );
	}
}

dlg_fileEdit::~dlg_fileEdit( )
{ /* not implemented yet*/ }

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
