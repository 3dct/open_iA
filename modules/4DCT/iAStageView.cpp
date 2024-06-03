// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAStageView.h"

#include "dlg_fileEdit.h"
#include "iA4DCTSettings.h"

#include <QPixmap>

iAStageView::iAStageView( QWidget* parent )
	: QWidget( parent )
	, m_data( nullptr )
{
	setupUi( this );
	lvImages->setStageView( this );
	connect( lForce, &iA4DCTForceWidget::valueChanged, this, &iAStageView::forceValueChanged);
}

iAStageView::~iAStageView( )
{

}

void iAStageView::setData( iA4DCTStageData * newData )
{
	m_data = newData;
}

iA4DCTStageData* iAStageView::getData( )
{
	return m_data;
}

void iAStageView::updateWidgets( )
{
	//this->lForce->setText( QString::number( m_data->Force ) );
	this->lForce->setValue( m_data->Force );

	QString thumbNail;
	if( m_data->getFilePath( S_4DCT_THUMB_NAME, thumbNail ) )
	{
		QPixmap pixmap( thumbNail );
		pixmap = pixmap.scaled( 600, 600, Qt::KeepAspectRatio );
		this->thumb->getBigPreview( )->setPixmap( pixmap );
		pixmap = pixmap.scaled( 200, 200, Qt::KeepAspectRatio );
		this->thumb->setPixmap( pixmap );
	}

	this->lvImages->updateData( );
}

void iAStageView::addFile( )
{
	dlg_fileEdit* fileEdit = new dlg_fileEdit( this );
	if( fileEdit->exec( ) != QDialog::Accepted ) {
		return;
	}

	iA4DCTFileData fileData;
	fileData.Name = fileEdit->getFileName( );
	fileData.Path = fileEdit->getFilePath( );
	m_data->Files.push_back( fileData );
	updateWidgets( );
}

void iAStageView::forceValueChanged( int val )
{
	m_data->Force = val;
}
