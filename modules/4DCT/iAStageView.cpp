/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAStageView.h"
// iA
#include "dlg_fileEdit.h"
#include "iA4DCTSettings.h"
// Qt
#include <QPixmap>

iAStageView::iAStageView( QWidget* parent /*= 0*/ )
	: QWidget( parent )
	, m_data( nullptr )
{
	setupUi( this );
	lvImages->setStageView( this );
	connect( lForce, SIGNAL( valueChanged( int ) ), this, SLOT( forceValueChanged( int ) ) );
}

iAStageView::~iAStageView( )
{

}

void iAStageView::setData( iA4DCTStageData * data )
{
	m_data = data;
}

iA4DCTStageData* iAStageView::getData( )
{
	return m_data;
}

void iAStageView::updateWidgets( )
{
	//this->lForce->setText( QString::number( m_data->Force ) );
	this->lForce->setValue( m_data->Force );

	QString thumb;
	if( m_data->getFilePath( S_4DCT_THUMB_NAME, thumb ) ) {
		QPixmap pixmap( thumb );
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
