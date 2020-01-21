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
#include "dlg_fractureView.h"

dlg_fractureView::dlg_fractureView( QWidget* parent /*= 0*/ )
	: QDialog( parent )
{
	setupUi( this );
	connect( cbStage, SIGNAL( currentIndexChanged( int ) ), this, SLOT( stageCurrentIndexChanged( int ) ) );
}

dlg_fractureView::dlg_fractureView( iA4DCTData * data, QWidget* parent /*= 0*/ )
	: dlg_fractureView( parent )
{
	m_data = data;

	for( auto d : *m_data ) {
		cbStage->addItem( QString::number( d->Force ) );
	}
}

dlg_fractureView::~dlg_fractureView( )
{ /* not implemented yet */ }

int dlg_fractureView::getStageIndex( )
{
	return cbStage->currentIndex( );
}

int dlg_fractureView::getImageIndex( )
{
	return cbMaskImg->currentIndex( );
}

void dlg_fractureView::stageCurrentIndexChanged( int ind )
{
	if( m_data->size( ) <= 0 )
		return;

	cbMaskImg->clear( );

	for( iA4DCTFileData f : m_data->at( ind )->Files ) {
		cbMaskImg->addItem( f.Name );
	}
}
