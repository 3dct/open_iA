/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
#include "iA4DCTDefectVisDockWidget.h"
// iA
#include "iADefectVisModule.h"
// vtk
#include <vtkRenderer.h>

iA4DCTDefectVisDockWidget::iA4DCTDefectVisDockWidget( QWidget * parent )
	: QDockWidget( parent )
{
	setupUi( this );
	connect( cbDefectVis, SIGNAL( colorChanged( QColor ) ), this, SLOT( changeColor( QColor ) ) );
	connect( sOpacity, SIGNAL( valueChanged( int ) ), this, SLOT( changeOpacity( int ) ) );
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
