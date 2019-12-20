/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAStabilityWidget.h"

#include "raycast/include/iADreamCasterCommon.h"

#include <QtMath>
#include <QMouseEvent>
#include <QPainter>

const float SCALE = 1.0f/3.5f;
iAStabilityWidget::iAStabilityWidget(QWidget *parent): QWidget(parent), m_pix_size(min_macro(parent->width(),parent->height()))
{
	m_parent = parent;
	m_pix_size*=SCALE;
	m_countX = 5;
	m_countY = 5;
	m_countZ = 5;
	m_stepPixSize = m_pix_size;
	m_spanAngleZ = 140.f;
	m_colsXY = new QColor*[2*m_countX+1];
	for (unsigned int i=0; i<2*m_countX+1; i++)
		m_colsXY[i] = new QColor[2*m_countY+1];
	for (unsigned int i=0; i<2*m_countX+1; i++)
		for (unsigned int j=0; j<2*m_countY+1; j++)
		{
			m_colsXY[i][j] = QColor(255,255,255);
		}
	m_colsZ = new QColor[2*m_countZ+1];
	m_colArrowX = QColor(255,255,255);
	m_colArrowY = QColor(255,255,255);
	m_colArrowZ = QColor(255,255,255);
}

iAStabilityWidget::~iAStabilityWidget()
{
	for (unsigned int i=0; i<m_countX; i++)
		if (m_colsXY[i])
			delete[] m_colsXY[i];
	if (m_colsXY)
		delete[] m_colsXY;
	if (m_colsZ)
		delete[] m_colsZ;
}

void iAStabilityWidget::paintEvent(QPaintEvent * /*event*/)
{
	m_pix_size = min_macro(m_parent->geometry().width(), m_parent->geometry().height());
	m_pix_size *= SCALE;
	m_stepPixSize = m_pix_size/(float)m_countX;
	//TODO: get the constants out
	unsigned int centerX = m_parent->geometry().width()/2;
	unsigned int centerY = m_parent->geometry().height()/2;
	unsigned int halfStep = (unsigned int)(m_stepPixSize/2);
	unsigned int shiftedCenterX = centerX-halfStep;
	unsigned int shiftedCenterY = centerY-halfStep;
	QPainter painter(this);
	if(painter.isActive() == false)
		return;
	//QFont font;
	//font.setPixelSize(18);
	//painter.setFont(font);
	QPen pen;
	pen.setCapStyle(Qt::RoundCap);
	pen.setCosmetic(true);
	pen.setWidthF(2.f);
	painter.setPen(pen);
	painter.setRenderHint(QPainter::Antialiasing, true);
	//fill background
	painter.fillRect(this->geometry(),QColor(150,150,150));
	//draw field
		QBrush brush(Qt::SolidPattern);
	unsigned int j=0, l=0;
	for (int i=-(int)m_countX; i<=(int)m_countX; i++, j++)
	{
		for (int k=-(int)m_countY, l=0; k<=(int)m_countY; k++, l++)
		{
			{
				if(i==0 || k==0)
					;//pen.setColor(QColor(0,0,0));
				else
				{
					pen.setColor(m_colsXY[j][l]);
					painter.setPen(pen);
					brush.setColor(m_colsXY[j][l]);
					painter.setBrush(brush);
					painter.drawRect((int)(shiftedCenterX+ m_stepPixSize*i), (int)(shiftedCenterY+ m_stepPixSize*k), (int)m_stepPixSize, (int)m_stepPixSize);
				}
			}
		}
	}
	j=0; l=0;
	for (int i=-(int)m_countX; i<=(int)m_countX; i++, j++)
	{
		for (int k=-(int)m_countY, l=0; k<=(int)m_countY; k++, l++)
		{
			{
				if(i==0 || k==0)
				{
					pen.setColor(QColor(0,0,0));
					painter.setPen(pen);
					brush.setColor(m_colsXY[j][l]);
					painter.setBrush(brush);
					painter.drawRect((int)(shiftedCenterX+ m_stepPixSize*i), (int)(shiftedCenterY+ m_stepPixSize*k), (int)m_stepPixSize, (int)m_stepPixSize);
				}
			}
		}
	}
	/*//axis cells
	j=0;
	for (int i=-m_countX; i<=(int)m_countX; i++, j++)
	{
		pen.setColor(QColor(0,0,0));
		painter.setPen(pen);
		brush.setColor(colsXY[j][0]);
		painter.setBrush(brush);
		painter.drawRect(shiftedCenterX+stepPixSize*i, shiftedCenterY, stepPixSize, stepPixSize);
	}
	l=0;
	for (int k=-m_countY, l=0; k<=(int)m_countY; k++, l++)
	{
		pen.setColor(QColor(0,0,0));
		painter.setPen(pen);
		brush.setColor(colsXY[0][l]);
		painter.setBrush(brush);
		painter.drawRect(shiftedCenterX, shiftedCenterY+stepPixSize*k, stepPixSize, stepPixSize);
	}*/
	//draw X axis
	pen.setColor(QColor(0,0,0));
	pen.setWidthF(3.f);
	painter.setPen(pen);
	QPointF tri[3];
	tri[0] = QPointF(shiftedCenterX+m_stepPixSize*(m_countX+1),centerY- m_stepPixSize);
	tri[1] = QPointF(shiftedCenterX+m_stepPixSize*(m_countX+2),centerY);
	tri[2] = QPointF(shiftedCenterX+m_stepPixSize*(m_countX+1),centerY+ m_stepPixSize);
	brush.setColor(m_colArrowX);
	painter.setBrush(brush);
	painter.drawConvexPolygon(tri, 3);
	painter.drawText((int)(shiftedCenterX+ m_stepPixSize*(m_countX+2)), (int)(centerY- m_stepPixSize), "rotX");
	//draw Y axis
	tri[0] = QPointF(centerX - m_stepPixSize, centerY + halfStep - m_stepPixSize*(m_countY+1));
	tri[1] = QPointF(centerX, centerY + halfStep - m_stepPixSize*(m_countY+2));
	tri[2] = QPointF(centerX + m_stepPixSize, centerY + halfStep - m_stepPixSize*(m_countY+1));
	brush.setColor(m_colArrowY);
	painter.setBrush(brush);
	painter.drawConvexPolygon(tri, 3);
	painter.drawText((int)(centerX+ m_stepPixSize+5),(int)(centerY + halfStep - m_stepPixSize*(m_countY+2)), "rotZ");
	//draw Z axis
/*
	brush.setStyle(Qt::NoBrush);
	painter.setBrush(brush);
	QPointF cellrect[4];
	float startAngle = -spanAngleZ/2;
	float deltaAngle = spanAngleZ/(2*m_countZ+1);
	float innerRad = (m_countX+1.5)*stepPixSize;
	float outerRad = (m_countX+2.5)*stepPixSize;
	float angle = startAngle;
	float radAng;
	float radAngDelta;
	for (int i=-m_countZ; i<=(int)m_countZ; i++, angle+=deltaAngle)
	{
		radAng = angle/180*M_PI;
		radAngDelta = (angle+deltaAngle)/180*M_PI;
		cellrect[0] =  QPointF(centerX + innerRad*sinf(radAng), centerY + innerRad*cosf(radAng));
		cellrect[1] =  QPointF(centerX + innerRad*sinf(radAngDelta), centerY + innerRad*cosf(radAngDelta));
		cellrect[2] =  QPointF(centerX + outerRad*sinf(radAngDelta), centerY + outerRad*cosf(radAngDelta));
		cellrect[3] =  QPointF(centerX + outerRad*sinf(radAng), centerY + outerRad*cosf(radAng));
		painter.drawConvexPolygon(cellrect, 4);
	}
	radAng = angle/180*M_PI;
	radAngDelta = (angle+deltaAngle)/180*M_PI;
	tri[0] = QPointF(centerX + (innerRad-halfStep)*sinf(radAng), centerY + (innerRad-halfStep)*cosf(radAng));
	tri[1] = QPointF(centerX + (innerRad+halfStep)*sinf(radAngDelta), centerY + (innerRad+halfStep)*cosf(radAngDelta));
	tri[2] = QPointF(centerX + (outerRad+halfStep)*sinf(radAng), centerY + (outerRad+halfStep)*cosf(radAng));
	brush.setColor(colArrowZ);
	painter.setBrush(brush);
	painter.drawConvexPolygon(tri, 3);
*/

	//QRectF innerRect(shiftedCenterX-stepPixSize*(m_countX+1), centerY - halfStep - stepPixSize*(m_countY+1), 2*(m_countX+1.5)*stepPixSize, 2*(m_countX+1.5)*stepPixSize);
	//QRectF outerRect(shiftedCenterX-stepPixSize*(m_countX+2), centerY - 3*halfStep - stepPixSize*(m_countY+1), 2*(m_countX+2.5)*stepPixSize, 2*(m_countX+2.5)*stepPixSize);
	//int startAng = 180*16;
	//int spanAng = 180*16;
	//painter.drawArc(innerRect, startAng, spanAng);
	//painter.drawArc(outerRect, startAng, spanAng);
	painter.end();
}

void iAStabilityWidget::mouseReleaseEvent ( QMouseEvent * event )
{
	m_lastX = event->x();
	m_lastY = event->y();
	mouseReleaseEventSignal();
}

void iAStabilityWidget::SetCount(int count)
{ 
	for (unsigned int i=0; i<m_countX; i++)
		if (m_colsXY[i])
			delete[] m_colsXY[i];
	if (m_colsXY)
		delete[] m_colsXY;

	m_countX = count; m_countY = count;	m_countZ = count;
	m_pix_size = min_macro(m_parent->geometry().width(), m_parent->geometry().height());
	m_pix_size*=SCALE;
	m_stepPixSize = m_pix_size/(float)count;

	m_colsXY = new QColor*[2*m_countX+1];
	for (unsigned int i=0; i<2*m_countX+1; i++)
		m_colsXY[i] = new QColor[2*m_countY+1];
	for (unsigned int i=0; i<2*m_countX+1; i++)
		for (unsigned int j=0; j<2*m_countY+1; j++)
		{
			m_colsXY[i][j] = QColor(255,255,255);
		}

	m_colArrowX = QColor(255,255,255);
	m_colArrowY = QColor(255,255,255);
	m_colArrowZ = QColor(255,255,255);
}
