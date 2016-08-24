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
	if(m_frameStyle != NO_FRAME)
	{
		qreal hw = m_penWidth * 0.5;
		QPointF points[4] = {
			QPointF(hw, hw),
			QPointF(width()-hw, hw ),
			QPointF(width()-hw, height()-hw),
			QPointF(hw,			height()-hw),
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
			painter.drawLine(points[0], points[3]);
			break;
		case FRAMED:
			for(int i=0; i<4; i++)
				painter.drawLine(points[i].x() + (i==0? m_penWidth : ((i==2)? -m_penWidth: 0)),
					points[i].y(),
					points[(i+1)%4].x() + (i==0? -m_penWidth : ((i==2)? m_penWidth: 0)),
					points[(i+1)%4].y());
			break;
		}
	}
	if (m_crossHair)
	{
		QPainter painter(this);
		QPen pen;
		pen.setColor(QColor(200, 200, 0, 125));
		pen.setWidthF(m_penWidth*0.5);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::HighQualityAntialiasing);
		painter.setPen(pen);
		const int CrossHairBarSize = 10;
		QPointF points[6] = {
			QPointF(width()/2-CrossHairBarSize, height()/2),
			QPointF(width()/2+CrossHairBarSize, height()/2),
			QPointF(width()/2, height()/2-CrossHairBarSize),
			QPointF(width()/2, height()/2-m_penWidth*0.5),
			QPointF(width()/2, height()/2+m_penWidth*0.5),
			QPointF(width()/2, height()/2+CrossHairBarSize),
		};
		painter.drawLine(points[0], points[1]);
		painter.drawLine(points[2], points[3]);
		painter.drawLine(points[4], points[5]);

	}
	QVTKWidget2::Frame();
}
	