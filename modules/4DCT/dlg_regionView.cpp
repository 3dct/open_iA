// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_regionView.h"

#include "dlg_4DCTFileOpen.h"

dlg_regionView::dlg_regionView( QWidget * parent )
	: QDialog( parent )
{
	setupUi( this );
	connect( pbSelectImage, &QPushButton::clicked, this, &dlg_regionView::onSelectButtonClicked);
}

void dlg_regionView::onSelectButtonClicked( )
{
	dlg_4DCTFileOpen dialog( this );
	dialog.setData( m_data );
	if (dialog.exec() != QDialog::Accepted)
	{
		return;
	}
	m_file = dialog.getFile( );
	lFilename->setText( m_file.Name );
}

void dlg_regionView::setData( iA4DCTData * newData )
{
	m_data = newData;
}

QString dlg_regionView::getImagePath( )
{
	return m_file.Path;
}

QString dlg_regionView::getImageName( )
{
	return m_file.Name;
}

double dlg_regionView::getThreshold( )
{
	return dspThreshold->value( );
}
