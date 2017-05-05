/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iATriangleButton.h"
#include "iAGEMSeConstants.h"

#include <QMouseEvent>
#include <QPainter>

iATriangleButton::iATriangleButton():
	m_expanded(false)
{
	setStyleSheet("background-color: transparent;");
}

void iATriangleButton::paintEvent(QPaintEvent* ev)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	QRect g(geometry());
	p.setPen(Qt::black);

	int partX = g.width()/4;
	int partY = partX*2;
	
	p.translate(g.width()/2, g.height()/2);
	if (m_expanded)
	{
		p.rotate(45);
	}

	QPolygon poly;
	poly<< QPoint(-partX, -partY)
		<< QPoint(partX, 0)
		<< QPoint(-partX,  partY);
	
	if (m_expanded)
	{
		QPainterPath path;
		path.addPolygon(poly);
		p.fillPath(path, DefaultColors::TriangleButtonSelectedBrush);
	}
	p.setPen(DefaultColors::TriangleButtonPen);
	p.drawPolygon(poly);
}

void iATriangleButton::mouseReleaseEvent(QMouseEvent * ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		Toggle();
		emit Clicked();
	}
}


bool iATriangleButton::Toggle()
{
	m_expanded = !m_expanded;
	update();
	return m_expanded;
}

bool iATriangleButton::IsExpanded() const
{
	return m_expanded;
}