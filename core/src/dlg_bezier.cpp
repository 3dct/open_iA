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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "dlg_bezier.h"

#include "iAHistogramWidget.h"

#include <vtkImageData.h>

#include <QPen>
#include <QPainter>
#include <QMouseEvent>

dlg_bezier::dlg_bezier(iADiagramFctWidget *fctDiagram, QColor &color, bool res)
	: dlg_function(fctDiagram)
{
	this->color = color;
	active = false;

	double range = fctDiagram->GetDataRange();

	controlDist = range/8;

	if (res)
		reset();

	selectedPoint = -1;
}

void dlg_bezier::draw(QPainter &painter)
{
	draw(painter, color, 3);
}

void dlg_bezier::draw(QPainter &painter, QColor color, int lineWidth)
{
	bool active = (fctDiagram->getSelectedFunction() == this);

	// draw line
	QPen pen = painter.pen();
	pen.setColor(color);
	pen.setWidth(lineWidth);

	painter.setPen(pen);
	
	int length = (int)realPoints.size();
	for(int l = 0; l < length-1; l+=3)
	{
		double X1 = realPoints[l].x();
		double Y1 = realPoints[l].y();

		// Variable
		double a = 1.0;
		double b = 1.0 - a;

		for (int i = 0; i <= 50; i++)
		{
			// Get a point on the curve
			double X2 = realPoints[l].x()*a*a*a + realPoints[l + 1].x() * 3 * a*a*b + realPoints[l + 2].x() * 3 * a*b*b + realPoints[l + 3].x()*b*b*b;
			double Y2 = realPoints[l].y()*a*a*a + realPoints[l + 1].y() * 3 * a*a*b + realPoints[l + 2].y() * 3 * a*b*b + realPoints[l + 3].y()*b*b*b;
			  
			// Draw the line from point to point (assuming OGL is set up properly)
			int x1, y1;
			x1 = d2iX(X1);
			y1 = d2iY(Y1);

			int x2, y2;
			x2 = d2iX(X2);
			y2 = d2iY(Y2);

			painter.drawLine(x1, y1, x2, y2);

			X1 = X2;
			Y1 = Y2;

			// Change the variable
			a -= 0.02;
			b = 1.0 - a;
		}
	}


	// draw point lines
	if (active)
	{
		int hue, sat, val, alpha;
		QColor penColor = color;
		penColor.getHsv(&hue, &sat, &val, &alpha);
		val >>= 1;
		penColor.setHsv(hue, sat, val, alpha);

		pen.setColor(penColor);
		pen.setWidth(1);
		
		painter.setPen(pen);
		
		for(int l = 0; l < length; l+=3)
		{
			int x, y;
			x = d2iX(realPoints[l].x());
			y = d2iY(realPoints[l].y());

			if (l-1 > 0)
			{
				int x1, y1;
				x1 = d2iX(realPoints[l-1].x());
				y1 = d2iY(realPoints[l-1].y());

				painter.drawLine(x, y, x1, y1);
			}
			
			if (l+1 < length)
			{
				int x1, y1;
				x1 = d2iX(realPoints[l+1].x());
				y1 = d2iY(realPoints[l+1].y());

				painter.drawLine(x, y, x1, y1);
			}

			
		}


		// draw points

		QColor currentColor;
		QColor redColor = QColor(255, 0, 0, 255);
		painter.setBrush(QBrush(color));
		painter.setPen(pen);

		for(int l = 0; l < length; l++)
		{
			int x, y;
			x = d2iX(realPoints[l].x());
			y = d2iY(realPoints[l].y());

			int radius, size;

			int mod = selectedPoint % 3;
			if (mod == 0 && (l == selectedPoint || l == selectedPoint -1 || l == selectedPoint +1) ||
				mod == 1 && (l == selectedPoint || l == selectedPoint -1 || l == selectedPoint -2) ||
				mod == 2 && (l == selectedPoint || l == selectedPoint +1 || l == selectedPoint +2))
			{
				currentColor = redColor;
				radius = iADiagramFctWidget::POINT_RADIUS;
				size = iADiagramFctWidget::POINT_SIZE;
			}
			else
			{
				currentColor = color;
				radius = iADiagramFctWidget::SELECTED_POINT_RADIUS;
				size  = iADiagramFctWidget::SELECTED_POINT_SIZE;
			}
			
			// is function point?
			if (l % 3 == 0)
			{
				pen.setWidth(3);
				pen.setColor(currentColor);
				painter.setPen(pen);
				painter.drawEllipse(x-radius, y-radius, size, size);
			}
			// is control point?
			else
			{
				x = d2iX(viewPoints[l].x());
				y = d2iY(viewPoints[l].y());

				pen.setWidth(1);
				pen.setColor(currentColor);
				painter.setPen(pen);
				painter.drawEllipse(x-radius/2, y-radius/2, size/2, size/2);
			}
		}
	}
}

int dlg_bezier::selectPoint(QMouseEvent *event, int *x)
{
	int lx = event->x();
	int ly = fctDiagram->geometry().height() -event->y() - fctDiagram->getBottomMargin();
	int index = -1;
	
	for (unsigned int pointIndex = 0; pointIndex < viewPoints.size(); pointIndex++)
	{
		
		int viewX, viewY;
		
		viewX = d2vX(viewPoints[pointIndex].x());
		viewY = d2vY(viewPoints[pointIndex].y());
					
		if ((pointIndex % 3 == 0 && lx >= viewX-iADiagramFctWidget::POINT_RADIUS && lx <= viewX+iADiagramFctWidget::POINT_RADIUS &&
			ly >= viewY-iADiagramFctWidget::POINT_RADIUS && ly <= viewY+iADiagramFctWidget::POINT_RADIUS) ||
			(lx >= viewX-iADiagramFctWidget::POINT_RADIUS/2 && lx <= viewX+iADiagramFctWidget::POINT_RADIUS/2 &&
			ly >= viewY-iADiagramFctWidget::POINT_RADIUS/2 && ly <= viewY+iADiagramFctWidget::POINT_RADIUS/2))

		{
			index = pointIndex;
			break;
		}

		if (x != NULL)
		{
			if (*x == viewX)
				*x = lx+1;
			else
				*x = lx;
		}
	}

	selectedPoint = index;
	length = 0;
	if (index != -1)
	{
		length = getLength(viewPoints[index], realPoints[index]);

		int functionPointIndex = getFunctionPointIndex(index);

		//is control point?
		if (functionPointIndex != index)
		{
			unsigned int oppositePointIndex;
			if (functionPointIndex +1 == selectedPoint) oppositePointIndex = functionPointIndex-1;
			else oppositePointIndex = functionPointIndex +1;

			QPointF functionPoint = realPoints[functionPointIndex];

			if (oppositePointIndex < realPoints.size())
			{
				QPointF oppositePoint = realPoints[oppositePointIndex];
				oppositeLength = getLength(functionPoint, oppositePoint);
			}
			else
				oppositeLength = 0;
		}
	}

	return index;
}

int dlg_bezier::addPoint(int x, int y)
{
	if (y < 0)
		y = 0;

	double xf = v2dX(x);
	
	int index = 0;
	
	std::vector<QPointF>::iterator it = realPoints.begin();
	while(it != realPoints.end() && it->x() < xf)
	{
		index++;
		it+=3;
	}

	insert(index, x, y);

	selectedPoint = index*3;
	
	return selectedPoint;
}

void dlg_bezier::removePoint(int index)
{
	std::vector<QPointF>::iterator it;
	
	for (int i = 0; i < 3; i++)
	{
		it = realPoints.begin();
		it += (index-1);
		realPoints.erase(it);

		it = viewPoints.begin();
		it += (index-1);
		viewPoints.erase(it);
	}
}

void dlg_bezier::moveSelectedPoint(int x, int y)
{
	if (isFunctionPoint(selectedPoint))
	{
		if (x < 0) x = 0;
		if (x > fctDiagram->geometry().width()-1) x = fctDiagram->geometry().width()-1;
		if (y < 0) y = 0;
		if (y > (fctDiagram->geometry().height() - fctDiagram->getBottomMargin()-1)*fctDiagram->getYZoom()) y = (fctDiagram->geometry().height() - fctDiagram->getBottomMargin()-1)*fctDiagram->getYZoom();

		if (isEndPoint(selectedPoint))
			y = 0;
	}


	int functionPointIndex = getFunctionPointIndex(selectedPoint);
	double dLength = getLength(viewPoints[functionPointIndex], viewPoints[selectedPoint]);
	double dx = 0;
	double dy = 0;

	if (dLength != 0)
	{
		dx = (d2vX(viewPoints[selectedPoint].x()) -d2vX(viewPoints[functionPointIndex].x())) /dLength;
		dy = (d2vY(viewPoints[selectedPoint].y()) -d2vY(viewPoints[functionPointIndex].y())) /dLength;
	}
	
	double vx, vy, fx, fy;
	vx = v2dX(x);
	vy = v2dY(y);
	fx = v2dX(x +dx*length);
	fy = v2dY(y +dy*length);
						
	QPointF &selPoint = realPoints[selectedPoint];
	bool functionPoint = isFunctionPoint(selectedPoint);
	if (functionPoint)
	{
		if (selectedPoint > 0)
		{
			QPointF &prevControlPoint = realPoints[selectedPoint-1];
			double diffX = selPoint.x() -prevControlPoint.x();
			double diffY = selPoint.y() -prevControlPoint.y();
			
			double pointX = fx -diffX;
			double pointY = fy -diffY;

			prevControlPoint.setX(pointX);
			prevControlPoint.setY(pointY);

			setViewPoint(selectedPoint-1);
		}
		if (selectedPoint < realPoints.size()-1)
		{
			QPointF &nextControlPoint = realPoints[selectedPoint+1];
			double diffX = selPoint.x() -nextControlPoint.x();
			double diffY = selPoint.y() -nextControlPoint.y();
			
			double pointX = fx -diffX;
			double pointY = fy -diffY;

			nextControlPoint.setX(pointX);
			nextControlPoint.setY(pointY);

			setViewPoint(selectedPoint+1);
		}
	}

	selPoint.setX(fx);
	selPoint.setY(fy);
	viewPoints[selectedPoint].setX(vx);
	viewPoints[selectedPoint].setY(vy);

	if (!functionPoint)
	{
		setOppositeViewPoint(selectedPoint);
	}
}

bool dlg_bezier::isEndPoint(int index)
{
	return (index == 0 || index == realPoints.size()-1);
}

bool dlg_bezier::isDeletable(int index)
{
	return (index % 3 == 0 && !isEndPoint(index));
}

void dlg_bezier::reset()
{
	double dataRange[2];
	fctDiagram->GetDataRange(dataRange);
	
	double start = dataRange[0];
	double end = dataRange[1];
	
	viewPoints.clear();
	realPoints.clear();
	
	viewPoints.push_back(QPointF(start, 0));
	viewPoints.push_back(QPointF(start+controlDist, 0.0));
	viewPoints.push_back(QPointF(end-controlDist, 0.0));
	viewPoints.push_back(QPointF(end, 0.0));

	realPoints.push_back(QPointF(start, 0));
	realPoints.push_back(QPointF(start+controlDist, 0.0));
	realPoints.push_back(QPointF(end-controlDist, 0.0));
	realPoints.push_back(QPointF(end, 0.0));

	selectedPoint = -1;
}

void dlg_bezier::mouseReleaseEvent(QMouseEvent*)
{
	if (selectedPoint != -1)
		setViewPoint(selectedPoint);
}

void dlg_bezier::push_back(double x, double y)
{
	realPoints.push_back(QPointF(x, y));
	viewPoints.push_back(QPointF(x, y));
}

bool dlg_bezier::isFunctionPoint(int point)
{
	return point % 3 == 0;
}

bool dlg_bezier::isControlPoint(int point)
{
	return !isFunctionPoint(point);
}

void dlg_bezier::insert(unsigned int index, unsigned int x, unsigned int y)
{
	double xf = v2dX(x);
	double yf = v2dY(y);

	if (realPoints.size() == 0)
	{
		reset();
	}
	else
	{
		std::vector<QPointF>::iterator vit = viewPoints.begin();
		std::vector<QPointF>::iterator rit = realPoints.begin();
		viewPoints.insert(vit+index*3-1, QPointF(xf-controlDist/fctDiagram->getZoom(), yf));
		realPoints.insert(rit+index*3-1, QPointF(xf-controlDist/fctDiagram->getZoom(), yf));
		vit = viewPoints.begin();
		rit = realPoints.begin();
		viewPoints.insert(vit+index*3, QPointF(xf, yf));
		realPoints.insert(rit+index*3, QPointF(xf, yf));
		vit = viewPoints.begin();
		rit = realPoints.begin();
		viewPoints.insert(vit+index*3+1, QPointF(xf+controlDist/fctDiagram->getZoom(), yf));
		realPoints.insert(rit+index*3+1, QPointF(xf+controlDist/fctDiagram->getZoom(), yf));
	}
}

void dlg_bezier::setViewPoint(int selectedPoint)
{
	if (selectedPoint != -1)
	{
		QPointF realPoint = realPoints[selectedPoint];

		double dataRange[2];
		fctDiagram->GetDataRange(dataRange);

		double pointX = realPoint.x();
		double pointY = realPoint.y();

		int functionPointIndex = getFunctionPointIndex(selectedPoint);
		if (functionPointIndex == selectedPoint) viewPoints[selectedPoint] = realPoint;
		else
		{
			QPointF functionPoint = realPoints[functionPointIndex];
			
			if (pointX < dataRange[0] || pointY < 0 || pointX > dataRange[1] || pointY > fctDiagram->getMax()/fctDiagram->getYZoom())
			{
				//calculate intersection with horizontal borders
				double dx = realPoints[selectedPoint].x() -functionPoint.x();
				double dy = realPoints[selectedPoint].y() -functionPoint.y();

				double t = ((pointY < 0 ? 0.0 : fctDiagram->getMax()/fctDiagram->getYZoom())-functionPoint.y())/dy;
				double x = functionPoint.x() +t*dx;

				//calculate intersection with vertical borders
				double y = functionPoint.y() +((pointX < dataRange[0] ? dataRange[0] : dataRange[1])-functionPoint.x())/dx*dy;

				if (x >= dataRange[0] && x <= dataRange[1] && t > 0)
				{
					viewPoints[selectedPoint].setX(x);
					viewPoints[selectedPoint].setY(pointY < 0 ? 0.0: fctDiagram->getMax()/fctDiagram->getYZoom());
				}
				else
				{
					viewPoints[selectedPoint].setX(pointX < dataRange[0] ? dataRange[0] : dataRange[1]);
					viewPoints[selectedPoint].setY(y);
				}
			}
			else
				viewPoints[selectedPoint] = realPoint;		
		}
	}
}

void dlg_bezier::setOppositeViewPoint(int selectedPoint)
{
	int functionPointIndex = getFunctionPointIndex(selectedPoint);
	unsigned int oppositePointIndex;
	if (functionPointIndex +1 == selectedPoint) oppositePointIndex = functionPointIndex-1;
	else oppositePointIndex = functionPointIndex +1;

	if (oppositePointIndex < realPoints.size())
	{
		QPointF functionPoint = realPoints[functionPointIndex];
		QPointF point = realPoints[selectedPoint];
		QPointF oppositePoint = realPoints[oppositePointIndex];

		point.setX(d2vX(point.x()));
		functionPoint.setX(d2vX(functionPoint.x()));
		oppositePoint.setX(d2vX(oppositePoint.x()));
		point.setY(d2vY(point.y()));
		functionPoint.setY(d2vY(functionPoint.y()));
		oppositePoint.setY(d2vY(oppositePoint.y()));


		double length = sqrt(pow(point.x() -functionPoint.x(), 2) +pow(point.y() -functionPoint.y(), 2));
		double dx = -(point.x() -functionPoint.x()) /length;
		double dy = -(point.y() -functionPoint.y()) /length;

		realPoints[oppositePointIndex].setX(v2dX(functionPoint.x() +dx*oppositeLength));
		realPoints[oppositePointIndex].setY(v2dY(functionPoint.y() +dy*oppositeLength));

		setViewPoint(oppositePointIndex);
	}
}

int dlg_bezier::getFunctionPointIndex(int index)
{
	int mod = index %3;
	switch(mod)
	{
	case 0: return index;
	case 1: return index-1;
	case 2: return index+1;
	}

	return -1;
}

double dlg_bezier::getLength(QPointF start, QPointF end)
{
	return sqrt(pow((double)(d2vX(end.x())-d2vX(start.x())), 2)+pow((double)(d2vY(end.y())-d2vY(start.y())), 2));
}

double dlg_bezier::v2dX(int x)
{
	double dataRange[2];
	fctDiagram->GetDataRange(dataRange);

	return ((double)(x-fctDiagram->getTranslationX()) / (double)fctDiagram->geometry().width() * (dataRange[1] - dataRange[0]) ) /fctDiagram->getZoom() + dataRange[0];
}

double dlg_bezier::v2dY(int y)
{
	return fctDiagram->GetCoordinateConverter()->Diagram2ScreenY(y) *fctDiagram->getMax() /fctDiagram->getYZoom();
}

int dlg_bezier::d2vX(double x)
{
	double dataRange[2];
	fctDiagram->GetDataRange(dataRange);

	return (int)((x -dataRange[0]) * (double)fctDiagram->geometry().width() / (dataRange[1] - dataRange[0])*fctDiagram->getZoom()) +fctDiagram->getTranslationX();
}

int dlg_bezier::d2vY(double y)
{
	return (int)(y /fctDiagram->getMax() *(double)(fctDiagram->geometry().height() - fctDiagram->getBottomMargin()-1) *fctDiagram->getYZoom());
}

int dlg_bezier::d2iX(double x)
{
	return d2vX(x) -fctDiagram->getTranslationX();
}

int dlg_bezier::d2iY(double y)
{
	return d2vY(y);
}
