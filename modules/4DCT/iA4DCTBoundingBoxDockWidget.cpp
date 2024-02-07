// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTBoundingBoxDockWidget.h"

#include "iABoundingBoxVisModule.h"

#include <vtkRenderer.h>

iA4DCTBoundingBoxDockWidget::iA4DCTBoundingBoxDockWidget( QWidget * parent )
	: QDockWidget( parent )
{
	setupUi( this );
	connect(cbBoundingBox, &iAColorBox::colorChanged, this, &iA4DCTBoundingBoxDockWidget::changeColor);
	connect(sWidth, &QSlider::valueChanged, this, &iA4DCTBoundingBoxDockWidget::changeLineWidth);

	// default values
	cbBoundingBox->setColor( QColor( 0xff, 0xff, 0xff ) );
}

void iA4DCTBoundingBoxDockWidget::attachTo( iABoundingBoxVisModule * boundingBox )
{
	m_boundingBox = boundingBox;
}

void iA4DCTBoundingBoxDockWidget::changeColor( const QColor & col )
{
	m_boundingBox->setColor( col.redF( ), col.greenF( ), col.blueF( ) );
	emit updateRenderWindow( );
}

void iA4DCTBoundingBoxDockWidget::changeLineWidth( int val )
{
	float width = static_cast<float>( val + 1 ) / sWidth->maximum( ) * 10.;
	m_boundingBox->setLineWidth( width );
	emit updateRenderWindow( );
}

void iA4DCTBoundingBoxDockWidget::setRenderer( vtkRenderer * renderer )
{
	m_renderer = renderer;
}
