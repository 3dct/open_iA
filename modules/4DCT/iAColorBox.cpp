// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAColorBox.h"

#include <QPainter>

iAColorBox::iAColorBox( QWidget * parent )
	: QWidget( parent )
{
	m_dialog.setCurrentColor( Qt::red );
	connect( &m_dialog, &QColorDialog::currentColorChanged, this, &iAColorBox::onCurrentColorChanged);
	setAutoFillBackground( true );
}

void iAColorBox::setColor( QColor col )
{
	m_color = col;
}

QColor iAColorBox::getColor( )
{
	return m_color;
}

void iAColorBox::paintEvent( QPaintEvent * )
{
	QPainter painter( this );
	painter.setPen( QColor( 0, 0, 0 ) );
	painter.setBrush( QBrush( m_color ) );
	painter.drawRect( 0, 0, this->width( ) - 1, this->height( ) - 1 );
}

void iAColorBox::mouseDoubleClickEvent( QMouseEvent * /*event*/ )
{
	m_dialog.open( );
}

void iAColorBox::onCurrentColorChanged( const QColor & col )
{
	m_color = col;
	emit colorChanged( m_color );
}
