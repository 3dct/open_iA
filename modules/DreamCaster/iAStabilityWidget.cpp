// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAStabilityWidget.h"

#include "raycast/include/iADreamCasterCommon.h"

#include <QtMath>
#include <QMouseEvent>
#include <QPainter>

namespace
{
	void deleteColors2D(QColor** colsXY, unsigned int countX)
	{
		for (unsigned int i = 0; i < countX; i++)
		{
			delete[] colsXY[i];
		}
		delete[] colsXY;
	}
	QColor** allocateColors2D(unsigned int countX, unsigned int countY)
	{
		auto colsXY = new QColor*[2 * countX + 1];
		for (unsigned int i = 0; i < 2 * countX + 1; i++)
		{
			colsXY[i] = new QColor[2 * countY + 1];
		}
		for (unsigned int i = 0; i < 2 * countX + 1; i++)
		{
			for (unsigned int j = 0; j < 2 * countY + 1; j++)
			{
				colsXY[i][j] = QColor(255, 255, 255);
			}
		}
		return colsXY;
	}
}

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
	m_colsXY = allocateColors2D(m_countX, m_countY);
	m_colArrowX = QColor(255,255,255);
	m_colArrowY = QColor(255,255,255);
	m_colArrowZ = QColor(255,255,255);
}

iAStabilityWidget::~iAStabilityWidget()
{
	deleteColors2D(m_colsXY, m_countX);
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
	if (painter.isActive() == false)
	{
		return;
	}
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
	for (int i=-(int)m_countX, j=0; i<=(int)m_countX; i++, j++)
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
	for (int i=-(int)m_countX, j=0; i<=(int)m_countX; i++, j++)
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
	for (int i=-m_countX, j=0; i<=(int)m_countX; i++, j++)
	{
		pen.setColor(QColor(0,0,0));
		painter.setPen(pen);
		brush.setColor(colsXY[j][0]);
		painter.setBrush(brush);
		painter.drawRect(shiftedCenterX+stepPixSize*i, shiftedCenterY, stepPixSize, stepPixSize);
	}
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
	Q_UNUSED(event);
	//m_lastX = event->x();
	//m_lastY = event->y();
	mouseReleaseEventSignal();
}

void iAStabilityWidget::SetCount(int count)
{
	m_countX = count; m_countY = count;	m_countZ = count;
	m_pix_size = min_macro(m_parent->geometry().width(), m_parent->geometry().height());
	m_pix_size*=SCALE;
	m_stepPixSize = m_pix_size/(float)count;

	deleteColors2D(m_colsXY, m_countX);
	m_colsXY = allocateColors2D(m_countX, m_countY);

	m_colArrowX = QColor(255,255,255);
	m_colArrowY = QColor(255,255,255);
	m_colArrowZ = QColor(255,255,255);
}
