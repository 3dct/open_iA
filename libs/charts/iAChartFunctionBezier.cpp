/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAChartWithFunctionsWidget.h"
#include "iAMapper.h"
#include "iAMathUtility.h"

#include <QPen>
#include <QPainter>
#include <QMouseEvent>

#include <cassert>

iAChartFunctionBezier::iAChartFunctionBezier(iAChartWithFunctionsWidget *chart, QColor &color, bool res):
	iAChartFunction(chart),
	m_color(color),
	m_selectedPoint(-1),
	m_controlDist(m_chart->xRange() / 8)
{
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

	if (!functionActive)
	{
		return;
	}

	// draw point lines
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
	int selectionCenter = ((m_selectedPoint+1) / 3) * 3; // center of selected triple of points
	for(int l = 0; l < pointNumber; l++)
	{
		bool selected = std::abs(l - selectionCenter) <= 1;
		bool isFunctionPoint = (l % 3 == 0); // is it a function point? (if false: control point)
		double sizeFactor = isFunctionPoint ? 1 : 0.5; // control points only shown with half size
		double pointX = isFunctionPoint ? m_realPoints[l].x() : m_viewPoints[l].x();
		double pointY = isFunctionPoint ? m_realPoints[l].y() : m_viewPoints[l].y();
		int x = m_chart->xMapper().srcToDst(pointX);
		int y = m_chart->yMapper().srcToDst(pointY);
		drawPoint(painter, x, y, selected, penColor, sizeFactor);
	}
}

int iAChartFunctionBezier::selectPoint(int mouseX, int mouseY)
{
	int index = -1;
	assert(m_realPoints.size() < std::numeric_limits<int>::max());
	assert(m_viewPoints.size() < std::numeric_limits<int>::max());

	int selectionCenter = ((m_selectedPoint + 1) / 3) * 3; // center of selected triple of points
	for (size_t pointIndex = 0; pointIndex < m_viewPoints.size(); ++pointIndex)
	{
		bool selected = std::abs(static_cast<int>(pointIndex) - selectionCenter) <= 1;
		int viewX = m_chart->data2MouseX(m_viewPoints[pointIndex].x());
		int viewY = m_chart->yMapper().srcToDst(m_viewPoints[pointIndex].y());
		int pointRadius = iAChartFunction::pointRadius(selected) / ((pointIndex % 3 == 0) ? 1 : 2);
		if (std::abs(mouseX - viewX) <= pointRadius && std::abs(mouseY - viewY) <= pointRadius)
		{
			index = static_cast<int>(pointIndex);
			break;
		}
	}

	m_selectedPoint = index;
	return index;
}

int iAChartFunctionBezier::addPoint(int mouseX, int mouseY)
{
	double xf = m_chart->mouse2DataX(mouseX);

	int index = 0;

	std::vector<QPointF>::iterator it = m_realPoints.begin();
	while(it != m_realPoints.end() && it->x() < xf)
	{
		++index;
		it+=3;
	}

	insert(index, mouseX, mouseY);

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

void iAChartFunctionBezier::moveSelectedPoint(int mouseX, int mouseY)
{
	assert(m_realPoints.size() < std::numeric_limits<int>::max());
	if (isFunctionPoint(m_selectedPoint))
	{
		mouseX = clamp(0, m_chart->chartWidth(), mouseX);
		mouseY = clamp(0, m_chart->chartHeight(), mouseY);
		if (isEndPoint(m_selectedPoint))
		{
			mouseY = 0;
		}
	}

	int functionPointIndex = getFunctionPointIndex(m_selectedPoint);
	double dLength = getLength(m_viewPoints[functionPointIndex], m_viewPoints[m_selectedPoint]);
	double dx = 0;
	double dy = 0;

	if (dLength != 0)
	{
		dx = m_viewPoints[m_selectedPoint].x() - m_viewPoints[functionPointIndex].x();
		dy = m_viewPoints[m_selectedPoint].y() - m_viewPoints[functionPointIndex].y();
	}

	double vx = m_chart->mouse2DataX(mouseX);
	double vy = m_chart->yMapper().dstToSrc(mouseY);
	double fx = vx + dx;
	double fy = vy + dy;
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
	m_realPoints[m_selectedPoint].setX(vx);
	m_realPoints[m_selectedPoint].setY(vy);
	setViewPoint(m_selectedPoint);
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
	m_viewPoints.push_back(QPointF(end  - m_controlDist, 0.0));
	m_viewPoints.push_back(QPointF(end, 0.0));

	m_realPoints.push_back(QPointF(start, 0));
	m_realPoints.push_back(QPointF(start+ m_controlDist, 0.0));
	m_realPoints.push_back(QPointF(end  - m_controlDist, 0.0));
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

void iAChartFunctionBezier::insert(unsigned int index, unsigned int mouseX, unsigned int mouseY)
{
	double xf = m_chart->mouse2DataX(mouseX);
	double yf = m_chart->yMapper().dstToSrc(mouseY);

	if (m_realPoints.size() == 0)
	{
		reset();
	}
	else
	{
		m_viewPoints.insert(m_viewPoints.begin()+index*3-1, QPointF(xf-m_controlDist/m_chart->xZoom(), yf));
		m_realPoints.insert(m_realPoints.begin()+index*3-1, QPointF(xf-m_controlDist/m_chart->xZoom(), yf));
		m_viewPoints.insert(m_viewPoints.begin()+index*3,   QPointF(xf, yf));
		m_realPoints.insert(m_realPoints.begin()+index*3,   QPointF(xf, yf));
		m_viewPoints.insert(m_viewPoints.begin()+index*3+1, QPointF(xf+m_controlDist/m_chart->xZoom(), yf));
		m_realPoints.insert(m_realPoints.begin()+index*3+1, QPointF(xf+m_controlDist/m_chart->xZoom(), yf));
	}
}

void iAChartFunctionBezier::setViewPoint(int selPntIdx)
{
	if (selPntIdx == -1)
	{
		return;
	}
	QPointF realPoint = m_realPoints[selPntIdx];

	double pointX = realPoint.x();
	double pointY = realPoint.y();

	int functionPointIndex = getFunctionPointIndex(selPntIdx);
	if (functionPointIndex == selPntIdx)
	{
		m_viewPoints[selPntIdx] = realPoint;
	}
	else
	{
		QPointF functionPoint = m_realPoints[functionPointIndex];

		if (pointX < m_chart->xBounds()[0] || pointY < 0 || pointX > m_chart->xBounds()[1] || pointY > m_chart->yBounds()[1]/m_chart->yZoom())
		{
			//calculate intersection with horizontal borders
			double dx = m_realPoints[selPntIdx].x() -functionPoint.x();
			double dy = m_realPoints[selPntIdx].y() -functionPoint.y();

			double t = ((pointY < 0 ? 0.0 : m_chart->yBounds()[1]/m_chart->yZoom())-functionPoint.y())/dy;
			double x = functionPoint.x() +t*dx;

			//calculate intersection with vertical borders
			double y = functionPoint.y() +((pointX < m_chart->xBounds()[0] ? m_chart->xBounds()[0] : m_chart->xBounds()[1])-functionPoint.x())/dx*dy;

			if (x >= m_chart->xBounds()[0] && x <= m_chart->xBounds()[1] && t > 0)
			{
				m_viewPoints[selPntIdx].setX(x);
				m_viewPoints[selPntIdx].setY(pointY < 0 ? 0.0: m_chart->yBounds()[1]/ m_chart->yZoom());
			}
			else
			{
				m_viewPoints[selPntIdx].setX(pointX < m_chart->xBounds()[0] ? m_chart->xBounds()[0] : m_chart->xBounds()[1]);
				m_viewPoints[selPntIdx].setY(y);
			}
		}
		else
		{
			m_viewPoints[selPntIdx] = realPoint;
		}
	}
}

void iAChartFunctionBezier::setOppositeViewPoint(int selPntIdx)
{
	int fctPntIdx = getFunctionPointIndex(selPntIdx);
	int oppPntIdx = fctPntIdx + ((fctPntIdx + 1 == selPntIdx) ? -1 : 1);
	if (oppPntIdx < 0 || oppPntIdx >= static_cast<int>(m_realPoints.size()))
	{
		return;
	}
	// due to anisotropic x/y mapping in chart, we need to do the set the opposite point in pixel space:
	QPointF fctPnt = m_realPoints[fctPntIdx];
	fctPnt.setX(m_chart->xMapper().srcToDst(fctPnt.x()));
	fctPnt.setY(m_chart->yMapper().srcToDst(fctPnt.y()));
	QPointF selPnt = m_realPoints[selPntIdx];
	selPnt.setX(m_chart->xMapper().srcToDst(selPnt.x()));
	selPnt.setY(m_chart->yMapper().srcToDst(selPnt.y()));
	QPointF oppPnt = m_realPoints[oppPntIdx];
	oppPnt.setX(m_chart->xMapper().srcToDst(oppPnt.x()));
	oppPnt.setY(m_chart->yMapper().srcToDst(oppPnt.y()));
	QPointF newPnt = fctPnt - ((selPnt - fctPnt) / getLength(selPnt, fctPnt) * getLength(fctPnt, oppPnt));
	m_realPoints[oppPntIdx].setX(m_chart->xMapper().dstToSrc(newPnt.x()));
	m_realPoints[oppPntIdx].setY(m_chart->yMapper().dstToSrc(newPnt.y()));
	setViewPoint(oppPntIdx);
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
	return sqrt(pow(end.x() - start.x(), 2)+pow(end.y() - start.y(), 2));
}
