// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_fractureView.h"

dlg_fractureView::dlg_fractureView( QWidget* parent /*= 0*/ )
	: QDialog( parent )
{
	setupUi( this );
	connect( cbStage, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_fractureView::stageCurrentIndexChanged);
}

dlg_fractureView::dlg_fractureView( iA4DCTData * data, QWidget* parent /*= 0*/ )
	: dlg_fractureView( parent )
{
	m_data = data;

	for( auto d : *m_data )
	{
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
	if (m_data->size() <= 0)
	{
		return;
	}
	cbMaskImg->clear( );
	for( iA4DCTFileData f : m_data->at( ind )->Files )
	{
		cbMaskImg->addItem( f.Name );
	}
}
