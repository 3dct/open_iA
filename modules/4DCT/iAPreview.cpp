// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAPreview.h"

#include <QMouseEvent>
#include <QCursor>

iAPreview::iAPreview( QWidget* parent ) :
	QLabel( parent )
{
	m_bigPreview = new QLabel;
	m_bigPreview->setWindowFlags( Qt::CustomizeWindowHint );
}

iAPreview::~iAPreview( )
{
	delete m_bigPreview;
}

QLabel* iAPreview::getBigPreview( )
{
	return m_bigPreview;
}

void iAPreview::mousePressEvent( QMouseEvent* event )
{
	if( event->button( ) == Qt::LeftButton ) {
		m_bigPreview->move( QCursor::pos( ) );
		m_bigPreview->show( );
	}
}

void iAPreview::mouseReleaseEvent( QMouseEvent* )
{
	if( !m_bigPreview->isHidden( ) ) {
		m_bigPreview->hide( );
	}
}
