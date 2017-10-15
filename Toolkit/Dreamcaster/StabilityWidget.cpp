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
#include "StabilityWidget.h"
#include <qmath.h>
#include "raycast/include/common.h"

const float SCALE = 1.0f/3.5f;
StabilityWidget::StabilityWidget(QWidget *parent): QWidget(parent), m_pix_size(min_macro(parent->width(),parent->height()))
{
	m_parent = parent;
	m_pix_size*=SCALE;
	m_countX = 5;
	m_countY = 5;
	m_countZ = 5;
	stepPixSize = m_pix_size;
	spanAngleZ = 140.f;
	colsXY = new QColor*[2*m_countX+1];
	for (unsigned int i=0; i<2*m_countX+1; i++)
		colsXY[i] = new QColor[2*m_countY+1];
	for (unsigned int i=0; i<2*m_countX+1; i++)
		for (unsigned int j=0; j<2*m_countY+1; j++)
		{
			colsXY[i][j] = QColor(255,255,255);
		}
	colsZ = new QColor[2*m_countZ+1];
	colArrowX = QColor(255,255,255);
	colArrowY = QColor(255,255,255);
	colArrowZ = QColor(255,255,255);
}

StabilityWidget::~StabilityWidget()
{
	for (unsigned int i=0; i<m_countX; i++)
		if (colsXY[i])
			delete[] colsXY[i];
	if (colsXY)
		delete[] colsXY;
	if (colsZ)
		delete[] colsZ;
}

void StabilityWidget::paintEvent(QPaintEvent *event)
{
	m_pix_size = min_macro(m_parent->geometry().width(), m_parent->geometry().height());
	m_pix_size *= SCALE;
	stepPixSize = m_pix_size/(float)m_countX;
	//TODO: get the constants out
	unsigned int centerX = m_parent->geometry().width()/2;
	unsigned int centerY = m_parent->geometry().height()/2;
	unsigned int halfStep = (unsigned int)(stepPixSize/2);
	unsigned int shiftedCenterX = centerX-halfStep;
	unsigned int shiftedCenterY = centerY-halfStep;
	painter.begin(this);
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
					pen.setColor(colsXY[j][l]);
					painter.setPen(pen);
					brush.setColor(colsXY[j][l]);
					painter.setBrush(brush);
					painter.drawRect((int)(shiftedCenterX+stepPixSize*i), (int)(shiftedCenterY+stepPixSize*k), (int)stepPixSize, (int)stepPixSize);
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
					brush.setColor(colsXY[j][l]);
					painter.setBrush(brush);
					painter.drawRect((int)(shiftedCenterX+stepPixSize*i), (int)(shiftedCenterY+stepPixSize*k), (int)stepPixSize, (int)stepPixSize);
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
	tri[0] = QPointF(shiftedCenterX+stepPixSize*(m_countX+1),centerY-stepPixSize);
	tri[1] = QPointF(shiftedCenterX+stepPixSize*(m_countX+2),centerY);
	tri[2] = QPointF(shiftedCenterX+stepPixSize*(m_countX+1),centerY+stepPixSize);
	brush.setColor(colArrowX);
	painter.setBrush(brush);
	painter.drawConvexPolygon(tri, 3);
	painter.drawText((int)(shiftedCenterX+stepPixSize*(m_countX+2)), (int)(centerY-stepPixSize), "rotX");
	//draw Y axis
	tri[0] = QPointF(centerX - stepPixSize, centerY + halfStep - stepPixSize*(m_countY+1));
	tri[1] = QPointF(centerX, centerY + halfStep - stepPixSize*(m_countY+2));
	tri[2] = QPointF(centerX + stepPixSize, centerY + halfStep - stepPixSize*(m_countY+1));
	brush.setColor(colArrowY);
	painter.setBrush(brush);
	painter.drawConvexPolygon(tri, 3);
	painter.drawText((int)(centerX+stepPixSize+5),(int)(centerY + halfStep - stepPixSize*(m_countY+2)), "rotZ");
	//draw Z axis
	/*brush.setStyle(Qt::NoBrush);
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
	painter.drawConvexPolygon(tri, 3);/**/

	//QRectF innerRect(shiftedCenterX-stepPixSize*(m_countX+1), centerY - halfStep - stepPixSize*(m_countY+1), 2*(m_countX+1.5)*stepPixSize, 2*(m_countX+1.5)*stepPixSize);
	//QRectF outerRect(shiftedCenterX-stepPixSize*(m_countX+2), centerY - 3*halfStep - stepPixSize*(m_countY+1), 2*(m_countX+2.5)*stepPixSize, 2*(m_countX+2.5)*stepPixSize);
	//int startAng = 180*16;
	//int spanAng = 180*16;
	//painter.drawArc(innerRect, startAng, spanAng);
	//painter.drawArc(outerRect, startAng, spanAng);
	painter.end();
}

void StabilityWidget::mouseReleaseEvent ( QMouseEvent * event )
{
	lastX = event->x();
	lastY = event->y();
	mouseReleaseEventSignal();
}

void StabilityWidget::SetCount(int count)
{ 
	for (unsigned int i=0; i<m_countX; i++)
		if (colsXY[i])
			delete[] colsXY[i];
	if (colsXY)
		delete[] colsXY;

	m_countX = count; m_countY = count;	m_countZ = count;
	m_pix_size = min_macro(m_parent->geometry().width(), m_parent->geometry().height());
	m_pix_size*=SCALE;
	stepPixSize = m_pix_size/(float)count;

	colsXY = new QColor*[2*m_countX+1];
	for (unsigned int i=0; i<2*m_countX+1; i++)
		colsXY[i] = new QColor[2*m_countY+1];
	for (unsigned int i=0; i<2*m_countX+1; i++)
		for (unsigned int j=0; j<2*m_countY+1; j++)
		{
			colsXY[i][j] = QColor(255,255,255);
		}

	colArrowX = QColor(255,255,255);
	colArrowY = QColor(255,255,255);
	colArrowZ = QColor(255,255,255);
}
