// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTDefectVisDockWidget.h"

#include "iADefectVisModule.h"

#include <vtkRenderer.h>

iA4DCTDefectVisDockWidget::iA4DCTDefectVisDockWidget( QWidget * parent )
	: QDockWidget( parent )
{
	setupUi( this );
	connect( cbDefectVis, &iAColorBox::colorChanged, this, &iA4DCTDefectVisDockWidget::changeColor);
	connect( sOpacity, &QSlider::valueChanged, this, &iA4DCTDefectVisDockWidget::changeOpacity);
}

void iA4DCTDefectVisDockWidget::attachTo( iADefectVisModule * defectVis )
{
	m_defectVis = defectVis;
}

void iA4DCTDefectVisDockWidget::setRenderer( vtkRenderer * renderer )
{
	m_renderer = renderer;
}

void iA4DCTDefectVisDockWidget::changeColor( const QColor & col )
{
	m_defectVis->setColor( col.redF( ), col.greenF( ), col.blueF( ) );
	emit updateRenderWindow( );
}

void iA4DCTDefectVisDockWidget::changeOpacity( int val )
{
	float opacity = static_cast<float>( val + 1 ) / sOpacity->maximum( );
	m_defectVis->setOpacity( opacity );
	emit updateRenderWindow( );
}
