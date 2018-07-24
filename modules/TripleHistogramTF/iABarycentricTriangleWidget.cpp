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

#include "iABarycentricTriangleWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>

#define _USE_MATH_DEFINES // necessary to use M_PI (with math.height)
#include <math.h>

// Debug
#include <QDebug>

// Constants
static const qreal RAD60 = M_PI / 3.0;
static const qreal SIN60 = sin(RAD60);
static const qreal ONE_DIV_SIN60 = 1.0 / SIN60;
static const qreal COS60 = 0.5;
static const qreal ONE_DIV_THREE = 1.0 / 3.0;

static const int CONTROL_POINT_RADIUS = 10;

iABarycentricTriangleWidget::iABarycentricTriangleWidget(QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */) :
	QOpenGLWidget(parent, f), m_controlPointBCoord(ONE_DIV_THREE, ONE_DIV_THREE)
{
	QLabel *triangle = new QLabel(this);
	triangle->setText("Triangle");

	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	mainLayout->addWidget(triangle);

	m_triangleBorderPen.setWidth(5);
	m_triangleBorderPen.setColor(Qt::black);

	m_controlPointBorderPen.setWidth(3);
	m_controlPointBorderPen.setColor(Qt::black);

	m_controlPointCrossPen.setWidth(2);
	m_controlPointCrossPen.setColor(Qt::black);

	m_triangleFillBrush.setColor(Qt::white);

	initializeControlPointPaths();
}

void iABarycentricTriangleWidget::initializeControlPointPaths()
{
	m_controlPointBorderPainterPath = QPainterPath();
	m_controlPointBorderPainterPath.addEllipse(m_controlPoint, CONTROL_POINT_RADIUS, CONTROL_POINT_RADIUS);

	m_controlPointCrossPainterPath = QPainterPath();
	m_controlPointCrossPainterPath.moveTo(
		m_controlPoint.x() - CONTROL_POINT_RADIUS,
		m_controlPoint.y()
	);
	m_controlPointCrossPainterPath.lineTo(
		m_controlPoint.x() + CONTROL_POINT_RADIUS,
		m_controlPoint.y()
	);
	m_controlPointCrossPainterPath.moveTo(
		m_controlPoint.x(),
		m_controlPoint.y() - CONTROL_POINT_RADIUS
	);
	m_controlPointCrossPainterPath.lineTo(
		m_controlPoint.x(),
		m_controlPoint.y() + CONTROL_POINT_RADIUS
	);
}

iABarycentricTriangleWidget::~iABarycentricTriangleWidget()
{}

void iABarycentricTriangleWidget::initializeGL()
{
	glClearColor(0.9, 0.9, 0.9, 1);
}

void iABarycentricTriangleWidget::resizeGL(int w, int h)
{
	updatePositions(w, h);
}

void iABarycentricTriangleWidget::mousePressEvent(QMouseEvent *event)
{
	int x = event->x();
	int y = event->y();

	// TODO: remove
	qDebug() << "Mouse pressed - X: " << x << " | Y: " << y;
	qDebug() << "Inside triangle? " << m_triangle.getBarycentricCoordinates(x, y)->isInside();

	updateControlPointPosition(*m_triangle.getBarycentricCoordinates(x, y), event->pos());

	update();
}

void iABarycentricTriangleWidget::mouseMoveEvent(QMouseEvent *event)
{
	int x = event->x();
	int y = event->y();

	// TODO: remove
	qDebug() << "Mouse moved - X: " << x << " | Y: " << y;
	qDebug() << "Inside triangle? " << m_triangle.getBarycentricCoordinates(x, y)->isInside();
	
	// This should only happen in a mouse DRAG, not mouse MOVE
	// However, Qt is weird. Therefore, this method is only called on a mouse DRAG
	// TODO: fix?
	updateControlPointPosition(*m_triangle.getBarycentricCoordinates(x, y), event->pos());

	update();
}

void iABarycentricTriangleWidget::updatePositions(int width, int height)
{
	int tw, th; // triangle width and triangle height
	if ((double)width / (double)height < ONE_DIV_SIN60) // ONE_DIV_SIN60 = width ratio
	{// TODO (line above): casting width and height - is that really the best?
		tw = width;
		th = (int)round(width * SIN60); // SIN60 = height ratio
	}
	else {
		tw = (int)round(height * ONE_DIV_SIN60); // ONE_DIV_SIN60 = width ratio
		th = height;
	}

	// triangle box
	int top, left, right, bottom, centerX;
	top = (height - th) / 2;
	left = (width - tw) / 2;
	right = left + tw;
	bottom = top + th;
	centerX = left + (tw / 2);

	m_triangle.setXa(left);
	m_triangle.setYa(bottom);
	m_triangle.setXb(centerX);
	m_triangle.setYb(top);
	m_triangle.setXc(right);
	m_triangle.setYc(bottom);

	m_trianglePainterPath = QPainterPath();
	m_trianglePainterPath.moveTo(left, bottom);
	m_trianglePainterPath.lineTo(centerX, top);
	m_trianglePainterPath.lineTo(right, bottom);
	m_trianglePainterPath.lineTo(left, bottom);

	//m_triangle.updateCartesianCoordinates(m_controlPoint, m_controlPointBCoord);
	//m_controlPointBorderPainterPath.translate(m_c)
	
	moveControlPointTo(*m_triangle.getCartesianCoordinates(m_controlPointBCoord));
}

void iABarycentricTriangleWidget::updateControlPointPosition(BCoord bCoord, QPoint newPos)
{
	if (bCoord.isInside())
	{
		m_controlPointBCoord = bCoord;
		moveControlPointTo(newPos);
	}
	else {
		// Do nothing for now
		// TODO: Set point to closes position inside the triangle
	}
}

void iABarycentricTriangleWidget::moveControlPointTo(QPoint newPos)
{
	m_controlPointOld.setX(m_controlPoint.x());
	m_controlPointOld.setY(m_controlPoint.y());

	m_controlPoint.setX(newPos.x());
	m_controlPoint.setY(newPos.y());

	int movx = m_controlPoint.x() - m_controlPointOld.x();
	int movy = m_controlPoint.y() - m_controlPointOld.y();
	m_controlPointBorderPainterPath.translate(movx, movy);
	m_controlPointCrossPainterPath.translate(movx, movy);

	qDebug() << "Moving control point by X=" << movx << " and Y=" << movy;
}

// ----------------------------------------------------------------------------------------------
// PAINT METHODS
// ----------------------------------------------------------------------------------------------

void iABarycentricTriangleWidget::paintGL()
{
	paintTriangleFill();
	paintTriangleBorder();
	paintControlPoint();
}

void iABarycentricTriangleWidget::paintTriangleBorder()
{
	// TODO: is it good to do "QPainter p(this)" multiple times?
	QPainter p(this);

	p.setPen(m_triangleBorderPen);
	p.drawPath(m_trianglePainterPath);
	//p.drawLine(m_triangle.getXa(), m_triangle.getYa(), m_triangle.getXb(), m_triangle.getYb());
	//p.drawLine(m_triangle.getXb(), m_triangle.getYb(), m_triangle.getXc(), m_triangle.getYc());
	//p.drawLine(m_triangle.getXc(), m_triangle.getYc(), m_triangle.getXa(), m_triangle.getYa());
}

void iABarycentricTriangleWidget::paintTriangleFill()
{
	QPainter p(this);

	p.fillPath(m_trianglePainterPath, m_triangleFillBrush);
}

void iABarycentricTriangleWidget::paintControlPoint()
{
	QPainter p(this);

	p.setPen(m_controlPointCrossPen);
	p.drawPath(m_controlPointCrossPainterPath);

	p.setPen(m_controlPointBorderPen);
	p.drawPath(m_controlPointBorderPainterPath);
}