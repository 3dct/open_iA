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
#include "dlg_gaussian.h"

#include "charts/iADiagramFctWidget.h"
#include "iAMapper.h"
#include "iAMathUtility.h"

#include <vtkMath.h>

#include <QPen>
#include <QPainter>
#include <QMouseEvent>

dlg_gaussian::dlg_gaussian(iADiagramFctWidget *chart, QColor &color, bool res): dlg_function(chart)
{
	this->color = color;
	active = false;

	sigma = 0.0;
	mean = 0.0;
	multiplier  = 0.0;
	selectedPoint = -1;

	if (res)
		reset();
}

void dlg_gaussian::draw(QPainter &painter)
{
	draw(painter, color, 3);
}

void dlg_gaussian::draw(QPainter &painter, QColor color, int lineWidth)
{
	bool active = (chart->getSelectedFunction() == this);
	// draw line
	QPen pen = painter.pen();
	pen.setColor(color);
	pen.setWidth(lineWidth);

	painter.setPen(pen);

	double range = chart->XRange();
	double startStep = range /100;
	double step = startStep;

	double X1 = chart->XBounds()[0];
	double X2 = X1;
	double Y1 = 1.0/(sigma*sqrt(2*vtkMath::Pi()))*exp(-pow((X2-mean)/sigma, 2)/2) *multiplier;
	double Y2 = Y1;

	double smallStep = std::max(6 * sigma / 100, 0.25*i2dX(1));
	while (X2 <= chart->XBounds()[1]+step && step > std::numeric_limits<double>::epsilon())
	{
		Y1 = Y2;
		Y2 = 1.0/(sigma*sqrt(2*vtkMath::Pi()))*exp(-pow((X2-mean)/sigma, 2)/2) *multiplier;

		int x1, y1;
		x1 = d2iX(X1);
		y1 = d2iY(Y1);

		int x2, y2;
		x2 = d2iX(X2);
		y2 = d2iY(Y2);

		painter.drawLine(x1, y1, x2, y2);

		X1 = X2;

		if (X2+startStep > mean-3*sigma && X1 < mean+3*sigma)
			step = smallStep;
		else
			step = startStep;

		X2 = X1 +step;
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

		int x, lx, rx, y;
		double meanValue = 1.0/(sigma*sqrt(2*vtkMath::Pi()))*multiplier;

		x = d2iX(mean);
		lx = d2iX(mean-sigma);
		rx = d2iX(mean+sigma);
		y = d2iY(meanValue);

		painter.drawLine(lx, y, rx, y);

		// draw points
		QColor redColor = QColor(255, 0, 0, 255);
		painter.setBrush(QBrush(color));
		painter.setPen(pen);

		int radius = iADiagramFctWidget::POINT_RADIUS;
		int size = iADiagramFctWidget::POINT_SIZE;

		pen.setWidth(3);
		pen.setColor(redColor);
		painter.setPen(pen);
		painter.drawEllipse(x-radius, y-radius, size, size);

		pen.setWidth(1);
		painter.setPen(pen);
		painter.drawEllipse(lx-radius/2, y-radius/2, size/2, size/2);
		painter.drawEllipse(rx-radius/2, y-radius/2, size/2, size/2);
	}
}

int dlg_gaussian::selectPoint(QMouseEvent *event, int*)
{
	int lx = event->x();
	int ly = chart->geometry().height() - event->y() - chart->BottomMargin();

	double meanValue = 1.0/(sigma*sqrt(2*vtkMath::Pi()));

	int viewXPoint = d2vX(mean);
	int viewYPoint = d2vY(meanValue*multiplier);

	int viewXLeftSigmaPoint  = d2vX(mean-sigma);
	int viewXRightSigmaPoint = d2vX(mean+sigma);

	if (lx >= viewXLeftSigmaPoint-iADiagramFctWidget::POINT_RADIUS/2 && lx <= viewXLeftSigmaPoint+iADiagramFctWidget::POINT_RADIUS/2 &&
		ly >= viewYPoint-iADiagramFctWidget::POINT_RADIUS/2 && ly <= viewYPoint+iADiagramFctWidget::POINT_RADIUS/2)
		selectedPoint = 1;
	else if (lx >= viewXRightSigmaPoint-iADiagramFctWidget::POINT_RADIUS/2 && lx <= viewXRightSigmaPoint+iADiagramFctWidget::POINT_RADIUS/2 &&
			ly >= viewYPoint-iADiagramFctWidget::POINT_RADIUS/2 && ly <= viewYPoint+iADiagramFctWidget::POINT_RADIUS/2)
		selectedPoint = 2;
	else if (lx >= viewXPoint-iADiagramFctWidget::POINT_RADIUS && lx <= viewXPoint+iADiagramFctWidget::POINT_RADIUS &&
			ly >= viewYPoint-iADiagramFctWidget::POINT_RADIUS && ly <= viewYPoint+iADiagramFctWidget::POINT_RADIUS)
		selectedPoint = 0;

	else
		selectedPoint = -1;

	return selectedPoint;
}

void dlg_gaussian::moveSelectedPoint(int x, int y)
{
	y = clamp(0, chart->geometry().height() - chart->BottomMargin() - 1, y);
	if (selectedPoint != -1)
	{
		switch(selectedPoint)
		{
			case 0:
			{
				x = clamp(0, chart->geometry().width() - 1, x);
				mean = v2dX(x);
			}
			break;
			case 1: case 2:
			{
				sigma = fabs(mean -v2dX(x));
				if (sigma <= std::numeric_limits<double>::epsilon())
					sigma = fabs(mean - v2dX(x+1));
			}
		}

		double meanValue = 1.0/(sigma*sqrt(2*vtkMath::Pi()))*chart->YZoom();
		multiplier  = (double)y /(chart->geometry().height() - chart->BottomMargin()-1)*chart->YBounds()[1] /meanValue;
	}
}

void dlg_gaussian::reset()
{}

void dlg_gaussian::setMultiplier(int multiplier)
{
	double meanValue = 1.0/(sigma*sqrt(2*vtkMath::Pi()))*chart->YZoom();
	this->multiplier = v2dY(multiplier) /meanValue;
}

// TODO: unify somewhere!
double dlg_gaussian::v2dX(int x)
{
	return ((double)(x-chart->XShift()) / (double)chart->geometry().width() * chart->XRange()) /chart->XZoom() + chart->XBounds()[0];
}

double dlg_gaussian::v2dY(int y)
{
	return chart->YMapper()->SrcToDest(y) *chart->YBounds()[1] /chart->YZoom();
}

int dlg_gaussian::d2vX(double x)
{
	return (int)((x - chart->XBounds()[0]) * (double)chart->geometry().width() / chart->XRange()*chart->XZoom()) +chart->XShift();
}

int dlg_gaussian::d2vY(double y)
{
	return (int)(y /chart->YBounds()[1] *(double)(chart->geometry().height() - chart->BottomMargin()-1) *chart->YZoom());
}

int dlg_gaussian::d2iX(double x)
{
	return d2vX(x) -chart->XShift();
}

int dlg_gaussian::d2iY(double y)
{
	return d2vY(y);
}

double dlg_gaussian::i2dX(int x)
{
	return ((double)x / (double)chart->geometry().width() * chart->XRange()) /chart->XZoom() + chart->XBounds()[0];
}
