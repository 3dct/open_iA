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
#include "iAFramedQVTKWidget2.h"


iAFramedQVTKWidget2::iAFramedQVTKWidget2(QWidget * parent, const QGLWidget * shareWidget, Qt::WindowFlags f)
	: QVTKWidget2(parent, shareWidget, f),
	m_penWidth(4.0),
	m_frameStyle(FRAMED),
	m_crossHair(false)
{
	setAutoFillBackground(false);
}


void iAFramedQVTKWidget2::SetCrossHair(bool enabled)
{
	m_crossHair = enabled;
}

void iAFramedQVTKWidget2::SetFrameWidth(qreal newWidth)
{
	m_penWidth = newWidth;
}

qreal iAFramedQVTKWidget2::GetFrameWidth() const
{
	return m_penWidth;
}

void iAFramedQVTKWidget2::SetFrameStyle(FrameStyle frameStyle)
{
	m_frameStyle = frameStyle;
}

iAFramedQVTKWidget2::FrameStyle iAFramedQVTKWidget2::GetFrameStyle() const
{
	return m_frameStyle;
}
	
void iAFramedQVTKWidget2::Frame()
{
	if(m_frameStyle != NO_FRAME && m_penWidth > 0)
	{
		qreal hw = m_penWidth * 0.5;
		const double RoundFix = 0.01;
		const double HeightWidthFix = static_cast<int>(m_penWidth) % 2;
		QPointF points[4] = {
			QPointF(1 + hw -RoundFix, 1 + hw -RoundFix),
			QPointF(HeightWidthFix + width()-hw +RoundFix, 1 + hw -RoundFix),
			QPointF(HeightWidthFix + width()-hw +RoundFix, HeightWidthFix + height()-hw +RoundFix),
			QPointF(1 + hw -RoundFix, HeightWidthFix + height()-hw +RoundFix),
		};
		QPainter painter(this);
		QPen pen;
		pen.setColor(QColor(200, 200, 0, 125));
		pen.setWidthF(m_penWidth);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::HighQualityAntialiasing);
		painter.setPen(pen);
		switch (m_frameStyle)
		{
		case LEFT_SIDE:
			pen.setColor(QColor(255, 255, 255, 255) );
			painter.setPen(pen);
			drawBorderRectangle(painter, points, m_penWidth);
			pen.setColor(QColor(200, 200, 0, 125));
			painter.setPen(pen);
			painter.drawLine(
				points[0]+QPointF(-1, m_penWidth-1),
				points[3]-QPointF(1, m_penWidth+HeightWidthFix));
			break;
		case FRAMED:
			drawBorderRectangle(painter, points, m_penWidth);
			break;
		}
	}
	if (m_crossHair)
	{
		double crossHairWidth = 2;
		QPainter painter(this);
		QPen pen;
		pen.setColor(QColor(200, 200, 0, 125));
		pen.setWidthF(crossHairWidth);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::HighQualityAntialiasing);
		painter.setPen(pen);
		const int CrossHairBarSize = 10;
		QPointF points[6] = {
			// TODO: get actual mouse position (important if magic lens "stuck" on the edge)
			QPointF(width()/2-CrossHairBarSize, height()/2),
			QPointF(width()/2+CrossHairBarSize, height()/2),
			QPointF(width()/2, height()/2-CrossHairBarSize),
			QPointF(width()/2, height()/2- crossHairWidth),
			QPointF(width()/2, height()/2+ crossHairWidth),
			QPointF(width()/2, height()/2+CrossHairBarSize),
		};
		painter.drawLine(points[0], points[1]);
		painter.drawLine(points[2], points[3]);
		painter.drawLine(points[4], points[5]);

	}
	QVTKWidget2::Frame();
}

void drawBorderRectangle(QPainter & painter, QPointF const points[4], int const borderWidth)
{
	for (int i = 0; i<4; i++)       // to avoid double-drawing in the corners
		painter.drawLine(
			points[i].x()           + (i == 0 ? borderWidth : (i == 2 ? -borderWidth : 0)),
			points[i].y(),
			points[(i + 1) % 4].x() + (i == 0 ? -borderWidth : (i == 2 ? borderWidth : 0)),
			points[(i + 1) % 4].y());
}
