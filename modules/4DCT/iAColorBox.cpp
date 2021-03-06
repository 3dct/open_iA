/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
