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
#include "iAChartFunctionBezier.h"

#include "charts/iAChartWithFunctionsWidget.h"
#include "iAMapper.h"
#include "iAMathUtility.h"

#include <vtkImageData.h>

#include <QPen>
#include <QPainter>
#include <QMouseEvent>

#include <cassert>

iAChartFunctionBezier::iAChartFunctionBezier(iAChartWithFunctionsWidget *chart, QColor &color, bool res):
	iAChartFunction(chart),
	m_color(color),
	m_selectedPoint(-1)
{
	m_controlDist = chart->xRange() / 8;
	if (res)
	{
		reset();
	}
}

void iAChartFunctionBezier::draw(QPainter &painter)
{
	draw(painter, m_color, LineWidthUnselected);
}

void iAChartFunctionBezier::draw(QPainter &painter, QColor penColor, int lineWidth)
{
	bool functionActive = (m_chart->selectedFunction() == this);

	// draw line
	QPen pen = painter.pen();
	pen.setColor(penColor);
	pen.setWidth(lineWidth);

	painter.setPen(pen);

	int pointNumber = static_cast<int>(m_realPoints.size());
	for(int l = 0; l < pointNumber-1; l+=3)
	{
		double X1 = m_realPoints[l].x();
		double Y1 = m_realPoints[l].y();

		// Variable
		double a = 1.0;
		int StepCount = 50;
		double stepSize = 1.0 / StepCount;

		for (int i = 0; i <= StepCount; i++)
		{
			double b = 1.0 - a;
			// Get a point on the curve
			double X2 = m_realPoints[l].x()*a*a*a + m_realPoints[l + 1].x() * 3 * a*a*b + m_realPoints[l + 2].x() * 3 * a*b*b + m_realPoints[l + 3].x()*b*b*b;
			double Y2 = m_realPoints[l].y()*a*a*a + m_realPoints[l + 1].y() * 3 * a*a*b + m_realPoints[l + 2].y() * 3 * a*b*b + m_realPoints[l + 3].y()*b*b*b;

			// Draw the line from point to point (assuming OGL is set up properly)
			int x1 = m_chart->xMapper().srcToDst(X1);
			int y1 = m_chart->yMapper().srcToDst(Y1);
			int x2 = m_chart->xMapper().srcToDst(X2);
			int y2 = m_chart->yMapper().srcToDst(Y2);
			painter.drawLine(x1, y1, x2, y2);

			X1 = X2;
			Y1 = Y2;

			a -= stepSize;
		}
	}


	// draw point lines
	if (functionActive)
	{
		int hue, sat, val, alpha;
		QColor newPenColor = penColor;
		newPenColor.getHsv(&hue, &sat, &val, &alpha);
		val >>= 1;
		newPenColor.setHsv(hue, sat, val, alpha);

		pen.setColor(newPenColor);
		pen.setWidth(1);

		painter.setPen(pen);

		for(int l = 0; l < pointNumber; l+=3)
		{
			int x = m_chart->xMapper().srcToDst(m_realPoints[l].x());
			int y = m_chart->yMapper().srcToDst(m_realPoints[l].y());
			if (l-1 > 0)
			{
				int x1 = m_chart->xMapper().srcToDst(m_realPoints[l-1].x());
				int y1 = m_chart->yMapper().srcToDst(m_realPoints[l-1].y());
				painter.drawLine(x, y, x1, y1);
			}
			if (l+1 < pointNumber)
			{
				int x1 = m_chart->xMapper().srcToDst(m_realPoints[l+1].x());
				int y1 = m_chart->yMapper().srcToDst(m_realPoints[l+1].y());
				painter.drawLine(x, y, x1, y1);
			}
		}

		// draw points
		QColor redColor = QColor(255, 0, 0, 255);
		painter.setBrush(QBrush(penColor));
		painter.setPen(pen);

		int selectionCenter = ((m_selectedPoint+1) / 3) * 3; // center of selected triple of points
		for(int l = 0; l < pointNumber; l++)
		{
			bool selected = std::abs(l - selectionCenter) <= 1;
			bool isFunctionPoint = (l % 3 == 0); // is it a function point? (if false: control point)
			QColor currentColor = selected ? redColor : penColor;
			int sizeRadiusDenominator = isFunctionPoint ? 1 : 2; // control points only shown with half size
			int radius = (selected ? iAChartWithFunctionsWidget::SELECTED_POINT_RADIUS : iAChartWithFunctionsWidget::POINT_RADIUS) / sizeRadiusDenominator;
			int size   = (selected ? iAChartWithFunctionsWidget::SELECTED_POINT_SIZE   : iAChartWithFunctionsWidget::POINT_SIZE) / sizeRadiusDenominator;
			int penWidth = isFunctionPoint ? 1 : 3;
			double pointX = isFunctionPoint ? m_realPoints[l].x() : m_viewPoints[l].x();
			double pointY = isFunctionPoint ? m_realPoints[l].y() : m_viewPoints[l].y();
			int x = m_chart->xMapper().srcToDst(pointX);
			int y = m_chart->yMapper().srcToDst(pointY);
			pen.setWidth(penWidth);
			pen.setColor(currentColor);
			painter.setPen(pen);
			painter.drawEllipse(x-radius, y-radius, size, size);
		}
	}
}

int iAChartFunctionBezier::selectPoint(QMouseEvent *event, int *x)
{
	int lx = event->x() - m_chart->leftMargin() - m_chart->xShift();
	int ly = m_chart->chartHeight() - event->y();
	int index = -1;
	assert(m_realPoints.size() < std::numeric_limits<int>::max());
	assert(m_viewPoints.size() < std::numeric_limits<int>::max());

	int selectionCenter = ((m_selectedPoint + 1) / 3) * 3; // center of selected triple of points
	for (size_t pointIndex = 0; pointIndex < m_viewPoints.size(); ++pointIndex)
	{
		bool selected = std::abs(static_cast<int>(pointIndex) - selectionCenter) <= 1;
		int viewX = m_chart->xMapper().srcToDst(m_viewPoints[pointIndex].x()) + m_chart->xShift();
		int viewY = m_chart->yMapper().srcToDst(m_viewPoints[pointIndex].y());
		int pointRadius = (selected ? iAChartWithFunctionsWidget::SELECTED_POINT_RADIUS : iAChartWithFunctionsWidget::POINT_RADIUS)
			/ ((pointIndex % 3 == 0) ? 1 : 2);
		if (std::abs(lx - viewX) <= pointRadius && std::abs(ly - viewY) <= pointRadius)
		{
			index = static_cast<int>(pointIndex);
			break;
		}

		if (x != nullptr)
		{
			if (*x == viewX)
			{
				*x = lx + 1;
			}
			else
			{
				*x = lx;
			}
		}
	}

	m_selectedPoint = index;
	m_length = 0;
	if (index != -1)
	{
		m_length = getLength(m_viewPoints[index], m_realPoints[index]);

		int functionPointIndex = getFunctionPointIndex(index);

		//is control point?
		if (functionPointIndex != index)
		{
			int oppositePointIndex;
			if (functionPointIndex + 1 == m_selectedPoint)
			{
				oppositePointIndex = functionPointIndex - 1;
			}
			else
			{
				oppositePointIndex = functionPointIndex + 1;
			}

			QPointF functionPoint = m_realPoints[functionPointIndex];

			if (oppositePointIndex >= 0 && oppositePointIndex < static_cast<int>(m_realPoints.size()))
			{
				QPointF oppositePoint = m_realPoints[oppositePointIndex];
				m_oppositeLength = getLength(functionPoint, oppositePoint);
			}
			else
			{
				m_oppositeLength = 0;
			}
		}
	}
	return index;
}

int iAChartFunctionBezier::addPoint(int x, int y)
{
	double xf = m_chart->xMapper().dstToSrc(x - m_chart->xShift());

	int index = 0;

	std::vector<QPointF>::iterator it = m_realPoints.begin();
	while(it != m_realPoints.end() && it->x() < xf)
	{
		++index;
		it+=3;
	}

	insert(index, x, y);

	m_selectedPoint = index*3;

	return m_selectedPoint;
}

void iAChartFunctionBezier::removePoint(int index)
{
	std::vector<QPointF>::iterator it;

	for (int i = 0; i < 3; i++)
	{
		it = m_realPoints.begin();
		it += (index-1);
		m_realPoints.erase(it);

		it = m_viewPoints.begin();
		it += (index-1);
		m_viewPoints.erase(it);
	}
}

void iAChartFunctionBezier::moveSelectedPoint(int x, int y)
{
	assert(m_realPoints.size() < std::numeric_limits<int>::max());
	if (isFunctionPoint(m_selectedPoint))
	{
		x = clamp(0, m_chart->geometry().width() - 1, x);
		y = clamp(0, static_cast<int>((m_chart->chartHeight() - 1)*m_chart->yZoom()), y);
		if (isEndPoint(m_selectedPoint))
		{
			y = 0;
		}
	}

	int functionPointIndex = getFunctionPointIndex(m_selectedPoint);
	double dLength = getLength(m_viewPoints[functionPointIndex], m_viewPoints[m_selectedPoint]);
	double dx = 0;
	double dy = 0;

	if (dLength != 0)
	{
		dx = (m_chart->xMapper().srcToDst(m_viewPoints[m_selectedPoint].x()) - m_chart->xMapper().srcToDst(m_viewPoints[functionPointIndex].x())) / dLength;
		dy = (m_chart->yMapper().srcToDst(m_viewPoints[m_selectedPoint].y()) - m_chart->yMapper().srcToDst(m_viewPoints[functionPointIndex].y())) / dLength;
	}

	double vx = m_chart->xMapper().dstToSrc(x - m_chart->xShift());
	double vy = m_chart->yMapper().dstToSrc(y);
	double fx = m_chart->xMapper().dstToSrc(x - m_chart->xShift() + dx * m_length);
	double fy = m_chart->yMapper().dstToSrc(y + dy * m_length);

	QPointF &selPoint = m_realPoints[m_selectedPoint];
	bool functionPoint = isFunctionPoint(m_selectedPoint);
	if (functionPoint)
	{
		if (m_selectedPoint > 0)
		{
			QPointF &prevControlPoint = m_realPoints[m_selectedPoint-1];
			double diffX = selPoint.x() -prevControlPoint.x();
			double diffY = selPoint.y() -prevControlPoint.y();

			double pointX = fx -diffX;
			double pointY = fy -diffY;

			prevControlPoint.setX(pointX);
			prevControlPoint.setY(pointY);

			setViewPoint(m_selectedPoint-1);
		}
		if (m_selectedPoint < static_cast<int>(m_realPoints.size())-1)
		{
			QPointF &nextControlPoint = m_realPoints[m_selectedPoint+1];
			double diffX = selPoint.x() -nextControlPoint.x();
			double diffY = selPoint.y() -nextControlPoint.y();

			double pointX = fx -diffX;
			double pointY = fy -diffY;

			nextControlPoint.setX(pointX);
			nextControlPoint.setY(pointY);

			setViewPoint(m_selectedPoint+1);
		}
	}

	selPoint.setX(fx);
	selPoint.setY(fy);
	m_viewPoints[m_selectedPoint].setX(vx);
	m_viewPoints[m_selectedPoint].setY(vy);

	if (!functionPoint)
	{
		setOppositeViewPoint(m_selectedPoint);
	}
}

bool iAChartFunctionBezier::isEndPoint(int index) const
{
	assert(m_realPoints.size() < std::numeric_limits<int>::max());
	return (index == 0 || index == static_cast<int>(m_realPoints.size())-1);
}

bool iAChartFunctionBezier::isDeletable(int index) const
{
	return (index % 3 == 0 && !isEndPoint(index));
}

void iAChartFunctionBezier::reset()
{
	double start = m_chart->xBounds()[0];
	double end = m_chart->xBounds()[1];

	m_viewPoints.clear();
	m_realPoints.clear();

	m_viewPoints.push_back(QPointF(start, 0));
	m_viewPoints.push_back(QPointF(start+ m_controlDist, 0.0));
	m_viewPoints.push_back(QPointF(end- m_controlDist, 0.0));
	m_viewPoints.push_back(QPointF(end, 0.0));

	m_realPoints.push_back(QPointF(start, 0));
	m_realPoints.push_back(QPointF(start+ m_controlDist, 0.0));
	m_realPoints.push_back(QPointF(end- m_controlDist, 0.0));
	m_realPoints.push_back(QPointF(end, 0.0));

	m_selectedPoint = -1;
}

QString iAChartFunctionBezier::name() const
{
	return QString("Bezier (%1 points)").arg( (m_realPoints.size() + 2) / 3 );
}

size_t iAChartFunctionBezier::numPoints() const
{
	return m_realPoints.size();
}

void iAChartFunctionBezier::mouseReleaseEvent(QMouseEvent*)
{
	if (m_selectedPoint != -1)
	{
		setViewPoint(m_selectedPoint);
	}
}

void iAChartFunctionBezier::push_back(double x, double y)
{
	m_realPoints.push_back(QPointF(x, y));
	m_viewPoints.push_back(QPointF(x, y));
}

bool iAChartFunctionBezier::isFunctionPoint(int point)
{
	return point % 3 == 0;
}

bool iAChartFunctionBezier::isControlPoint(int point)
{
	return !isFunctionPoint(point);
}

void iAChartFunctionBezier::insert(unsigned int index, unsigned int x, unsigned int y)
{
	double xf = m_chart->xMapper().dstToSrc(x - m_chart->xShift());
	double yf = m_chart->yMapper().dstToSrc(y);

	if (m_realPoints.size() == 0)
	{
		reset();
	}
	else
	{
		std::vector<QPointF>::iterator vit = m_viewPoints.begin();
		std::vector<QPointF>::iterator rit = m_realPoints.begin();
		m_viewPoints.insert(vit+index*3-1, QPointF(xf-m_controlDist/m_chart->xZoom(), yf));
		m_realPoints.insert(rit+index*3-1, QPointF(xf-m_controlDist/m_chart->xZoom(), yf));
		vit = m_viewPoints.begin();
		rit = m_realPoints.begin();
		m_viewPoints.insert(vit+index*3, QPointF(xf, yf));
		m_realPoints.insert(rit+index*3, QPointF(xf, yf));
		vit = m_viewPoints.begin();
		rit = m_realPoints.begin();
		m_viewPoints.insert(vit+index*3+1, QPointF(xf+m_controlDist/m_chart->xZoom(), yf));
		m_realPoints.insert(rit+index*3+1, QPointF(xf+m_controlDist/m_chart->xZoom(), yf));
	}
}

void iAChartFunctionBezier::setViewPoint(int selectedPoint)
{
	if (selectedPoint != -1)
	{
		QPointF realPoint = m_realPoints[selectedPoint];

		double pointX = realPoint.x();
		double pointY = realPoint.y();

		int functionPointIndex = getFunctionPointIndex(selectedPoint);
		if (functionPointIndex == selectedPoint)
		{
			m_viewPoints[selectedPoint] = realPoint;
		}
		else
		{
			QPointF functionPoint = m_realPoints[functionPointIndex];

			if (pointX < m_chart->xBounds()[0] || pointY < 0 || pointX > m_chart->xBounds()[1] || pointY > m_chart->yBounds()[1]/m_chart->yZoom())
			{
				//calculate intersection with horizontal borders
				double dx = m_realPoints[selectedPoint].x() -functionPoint.x();
				double dy = m_realPoints[selectedPoint].y() -functionPoint.y();

				double t = ((pointY < 0 ? 0.0 : m_chart->yBounds()[1]/m_chart->yZoom())-functionPoint.y())/dy;
				double x = functionPoint.x() +t*dx;

				//calculate intersection with vertical borders
				double y = functionPoint.y() +((pointX < m_chart->xBounds()[0] ? m_chart->xBounds()[0] : m_chart->xBounds()[1])-functionPoint.x())/dx*dy;

				if (x >= m_chart->xBounds()[0] && x <= m_chart->xBounds()[1] && t > 0)
				{
					m_viewPoints[selectedPoint].setX(x);
					m_viewPoints[selectedPoint].setY(pointY < 0 ? 0.0: m_chart->yBounds()[1]/ m_chart->yZoom());
				}
				else
				{
					m_viewPoints[selectedPoint].setX(pointX < m_chart->xBounds()[0] ? m_chart->xBounds()[0] : m_chart->xBounds()[1]);
					m_viewPoints[selectedPoint].setY(y);
				}
			}
			else
			{
				m_viewPoints[selectedPoint] = realPoint;
			}
		}
	}
}

void iAChartFunctionBezier::setOppositeViewPoint(int selectedPoint)
{
	int functionPointIndex = getFunctionPointIndex(selectedPoint);
	int oppositePointIndex = functionPointIndex + ((functionPointIndex + 1 == selectedPoint) ? -1 : 1);

	if (oppositePointIndex < 0 || oppositePointIndex >= static_cast<int>(m_realPoints.size()))
	{
		return;
	}
	QPointF functionPoint = m_realPoints[functionPointIndex];
	QPointF point = m_realPoints[selectedPoint];
	QPointF oppositePoint = m_realPoints[oppositePointIndex];

	point.setX(m_chart->xMapper().srcToDst(point.x()));
	functionPoint.setX(m_chart->xMapper().srcToDst(functionPoint.x()));
	oppositePoint.setX(m_chart->xMapper().srcToDst(oppositePoint.x()));
	point.setY(m_chart->yMapper().srcToDst(point.y()));
	functionPoint.setY(m_chart->yMapper().srcToDst(functionPoint.y()));
	oppositePoint.setY(m_chart->yMapper().srcToDst(oppositePoint.y()));


	double curLength = sqrt(pow(point.x() -functionPoint.x(), 2) +pow(point.y() -functionPoint.y(), 2));
	double dx = -(point.x() -functionPoint.x()) / curLength;
	double dy = -(point.y() -functionPoint.y()) / curLength;

	m_realPoints[oppositePointIndex].setX(
		m_chart->xMapper().dstToSrc(functionPoint.x() + dx * m_oppositeLength - m_chart->xShift()));
	m_realPoints[oppositePointIndex].setY(m_chart->yMapper().dstToSrc(functionPoint.y() + dy * m_oppositeLength));

	setViewPoint(oppositePointIndex);
}

int iAChartFunctionBezier::getFunctionPointIndex(int index)
{
	int mod = index % 3;
	switch(mod)
	{
	case 0: return index;
	case 1: return index-1;
	case 2: return index+1;
	}
	return -1;
}

double iAChartFunctionBezier::getLength(QPointF start, QPointF end)
{
	return sqrt(pow(m_chart->xMapper().srcToDst(end.x()) - m_chart->xMapper().srcToDst(start.x()), 2)+
				pow(m_chart->yMapper().srcToDst(end.y()) - m_chart->yMapper().srcToDst(start.y()), 2));
}
