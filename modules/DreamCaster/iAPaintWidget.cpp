/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAPaintWidget.h"

iAPaintWidget::iAPaintWidget(QPixmap *a_pxmp, QWidget *parent): QWidget(parent)
{
	m_pxmp = a_pxmp;
	m_scale = 1;
	m_offset[0]=0; m_offset[1]=0;
	scaleCoef=0.1f;
	m_maxScale = 20.f;
	highlightCount=0;
	highlightPenWidth=1.f;
	m_highlightX=0;
	m_highlightY=0;
	QPainter painter;
}

iAPaintWidget::~iAPaintWidget()
{
	if(m_highlightX)
		delete [] m_highlightX;
	if(m_highlightY)
		delete [] m_highlightY;
}

void iAPaintWidget::paintEvent(QPaintEvent * /*event*/)
{
	QPainter painter;
	QPointF highlightPts[4];
	float geom[2] = {(float)geometry().width(), (float)geometry().height()};
	float pxmpGeom[2] = {(float)m_pxmp->width(), (float)m_pxmp->height()};
	float scale[2] = {geom[0]/pxmpGeom[0], geom[1]/pxmpGeom[1]};
	float hx, hy;
	QPen pen;
	painter.begin(this);
	painter.drawPixmap(QRectF(m_offset[0]*m_scale, m_offset[1]*m_scale, geometry().width()*m_scale, geometry().height()*m_scale), *m_pxmp, QRectF(0, 0, (double)m_pxmp->width(),(double)m_pxmp->height()));
	//highligh highlighted indices
	pen.setColor(highlightColor);
	pen.setWidthF(highlightPenWidth);
	painter.setPen(pen);
	for (int i=0; i<highlightCount; i++)
	{
		hx = m_highlightX[i]*scale[0];
		hy = m_highlightY[i]*scale[1];
		highlightPts[0] = QPointF((m_offset[0]+hx)*m_scale,(m_offset[1]+hy)*m_scale);
		highlightPts[1] = QPointF((m_offset[0]+hx+scale[0])*m_scale,(m_offset[1]+hy)*m_scale);
		highlightPts[2] = QPointF((m_offset[0]+hx+scale[0])*m_scale,(m_offset[1]+hy+scale[1])*m_scale);
		highlightPts[3] = QPointF((m_offset[0]+hx)*m_scale,(m_offset[1]+hy+scale[1])*m_scale);
		painter.drawPolygon(highlightPts, 4);
	}
	painter.end();
}

void iAPaintWidget::mouseReleaseEvent ( QMouseEvent * event )
{
	lastX = (int)(((double)event->x())/m_scale - m_offset[0]);
	lastY = (int)(((double)event->y())/m_scale - m_offset[1]);
	mouseReleaseEventSignal();
	if( event->button()&Qt::LeftButton && !(event->buttons()&Qt::RightButton) )
		mouseReleaseEventSignal(lastX, lastY);
}

void iAPaintWidget::mousePressEvent ( QMouseEvent * event )
{
	m_lastMoveX = event->x();
	m_lastMoveY = event->y();
	lastMoveX = (int)(((double)event->x())/m_scale - m_offset[0]);
	lastMoveY = (int)(((double)event->y())/m_scale - m_offset[1]);
	mousePressEventSignal();
}

void iAPaintWidget::mouseMoveEvent ( QMouseEvent * event )
{
	if(event->buttons()&Qt::RightButton && event->buttons()&Qt::LeftButton)//scaling
	{
		double delta = scaleCoef*((double)event->y()-(double)m_lastMoveY);
		//m_offset[0] += (double)geometry().width()/m_scale - (double)geometry().width()/(m_scale+delta);
		//m_offset[1] += (double)geometry().height()/m_scale - (double)geometry().height()/(m_scale+delta);
		m_scale += delta;
		checkScale();
		checkOffset();
		update();
		ChangedSignal(m_scale, m_offset[0], m_offset[1]);
	}
	else if(event->buttons()&Qt::RightButton)//moving
	{
		m_offset[0] += ((double)event->x()-m_lastMoveX)/m_scale;
		m_offset[1] += ((double)event->y()-m_lastMoveY)/m_scale;
		checkOffset();
		update();
		ChangedSignal(m_scale, m_offset[0], m_offset[1]);
	}
	m_lastMoveX = event->x();
	m_lastMoveY = event->y();
	lastMoveX = (int)(((double)event->x())/m_scale - m_offset[0]);
	lastMoveY = (int)(((double)event->y())/m_scale - m_offset[1]);
	mouseMoveEventSignal();
}
void iAPaintWidget::checkOffset()
{
	if(m_offset[0]>0)
		m_offset[0] = 0;
	if(m_offset[1]>0)
		m_offset[1] = 0;
	if(m_offset[0]<-(double)geometry().width()+(double)geometry().width()/m_scale)
		m_offset[0] = -(double)geometry().width()+(double)geometry().width()/m_scale;
	if(m_offset[1]<-(double)geometry().height()+(double)geometry().height()/m_scale)
		m_offset[1] = -(double)geometry().height()+(double)geometry().height()/m_scale;
}
void iAPaintWidget::checkScale()
{
	if(m_scale<1.f)
		m_scale = 1.f;
	if(m_scale>m_maxScale)
		m_scale = m_maxScale;
}
void iAPaintWidget::SetHiglightedIndices(int *inds_x, int *inds_y, unsigned int count)
{
	RemoveHighlights();
	highlightCount = count;
	m_highlightX = new int[highlightCount];
	m_highlightY = new int[highlightCount];
	for (int i=0; i<highlightCount; i++)
	{
		m_highlightX[i] = inds_x[i];
		m_highlightY[i] = inds_y[i];
	}
}

void iAPaintWidget::SetHighlightStyle(const QColor &color, float penWidth)
{
	highlightColor = QColor(color);
	highlightPenWidth = penWidth;
}

void iAPaintWidget::RemoveHighlights()
{
	if(m_highlightX)
	{
		delete [] m_highlightX;
		m_highlightX = 0;
	}
	if(m_highlightY)
	{
		delete [] m_highlightY;
		m_highlightY = 0;
	}
	highlightCount = 0;
}

void iAPaintWidget::UpdateSlot(double & scale, double & offsetX, double & offsetY)
{
	m_scale = scale;
	m_offset[0] = offsetX;
	m_offset[1] = offsetY;
	update();
}
