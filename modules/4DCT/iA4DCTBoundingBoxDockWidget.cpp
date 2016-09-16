/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "iA4DCTBoundingBoxDockWidget.h"
// iA
#include "iABoundingBoxVisModule.h"
// vtk
#include <vtkRenderer.h>

iA4DCTBoundingBoxDockWidget::iA4DCTBoundingBoxDockWidget( QWidget * parent )
	: QDockWidget( parent )
{
	setupUi( this );
	connect( cbBoundingBox, SIGNAL( colorChanged( QColor ) ), this, SLOT( changeColor( QColor ) ) );
	connect( sWidth, SIGNAL( valueChanged( int ) ), this, SLOT( changeLineWidth( int ) ) );

	
	// default values
	cbBoundingBox->setColor( QColor( 0xff, 0xff, 0xff ) );
}

void iA4DCTBoundingBoxDockWidget::attachTo( iABoundingBoxVisModule * boundingBox )
{
	m_boundingBox = boundingBox;
}

void iA4DCTBoundingBoxDockWidget::changeColor( const QColor & col )
{
	m_boundingBox->setColor( col.redF(), col.greenF(), col.blueF() );
	emit updateRenderWindow();
}

void iA4DCTBoundingBoxDockWidget::changeLineWidth( int val )
{
	float width = static_cast<float>(val + 1) / sWidth->maximum() * 10.;
	m_boundingBox->setLineWidth( width );
	emit updateRenderWindow();
}

void iA4DCTBoundingBoxDockWidget::setRenderer( vtkRenderer * renderer )
{
	m_renderer = renderer;
}
