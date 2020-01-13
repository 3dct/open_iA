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
#include "iAChartFunctionGaussian.h"

#include "charts/iAChartWithFunctionsWidget.h"
#include "iAMapper.h"
#include "iAMathUtility.h"

#include <vtkMath.h>

#include <QPen>
#include <QPainter>
#include <QMouseEvent>

iAChartFunctionGaussian::iAChartFunctionGaussian(iAChartWithFunctionsWidget *chart, QColor &color, bool res):
	iAChartFunction(chart),
	m_color(color),
	m_sigma(0.0),
	m_mean(0.0),
	m_multiplier(0.0),
	m_selectedPoint(-1)
{
	if (res)
		reset();
}

void iAChartFunctionGaussian::draw(QPainter &painter)
{
	draw(painter, m_color, 3);
}

void iAChartFunctionGaussian::draw(QPainter &painter, QColor color, int lineWidth)
{
	bool active = (chart->selectedFunction() == this);
	// draw line
	QPen pen = painter.pen();
	pen.setColor(color);
	pen.setWidth(lineWidth);

	painter.setPen(pen);

	double range = chart->xRange();
	double startStep = range /100;
	double step = startStep;

	double X1 = chart->xBounds()[0];
	double X2 = X1;
	double Y1 = 1.0/(m_sigma*sqrt(2*vtkMath::Pi()))*exp(-pow((X2 - m_mean)/ m_sigma, 2)/2) * m_multiplier;
	double Y2 = Y1;

	double smallStep = std::max(6 * m_sigma / 100, 0.25*i2dX(1));
	while (X2 <= chart->xBounds()[1]+step && step > std::numeric_limits<double>::epsilon())
	{
		Y1 = Y2;
		Y2 = 1.0/(m_sigma*sqrt(2*vtkMath::Pi()))*exp(-pow((X2 - m_mean)/ m_sigma, 2)/2) * m_multiplier;

		int x1, y1;
		x1 = d2iX(X1);
		y1 = d2iY(Y1);

		int x2, y2;
		x2 = d2iX(X2);
		y2 = d2iY(Y2);

		painter.drawLine(x1, y1, x2, y2);

		X1 = X2;

		if (X2+startStep > m_mean-3* m_sigma && X1 < m_mean+3* m_sigma)
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
		double meanValue = 1.0/(m_sigma*sqrt(2*vtkMath::Pi()))*m_multiplier;

		x = d2iX(m_mean);
		lx = d2iX(m_mean - m_sigma);
		rx = d2iX(m_mean + m_sigma);
		y = d2iY(meanValue);

		painter.drawLine(lx, y, rx, y);

		// draw points
		QColor redColor = QColor(255, 0, 0, 255);
		painter.setBrush(QBrush(color));
		painter.setPen(pen);

		int radius = iAChartWithFunctionsWidget::POINT_RADIUS;
		int size = iAChartWithFunctionsWidget::POINT_SIZE;

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

int iAChartFunctionGaussian::selectPoint(QMouseEvent *event, int*)
{
	int lx = event->x();
	int ly = chart->geometry().height() - event->y() - chart->bottomMargin();

	double meanValue = 1.0/(m_sigma*sqrt(2*vtkMath::Pi()));

	int viewXPoint = d2vX(m_mean);
	int viewYPoint = d2vY(meanValue*m_multiplier);

	int viewXLeftSigmaPoint  = d2vX(m_mean-m_sigma);
	int viewXRightSigmaPoint = d2vX(m_mean+m_sigma);

	if (lx >= viewXLeftSigmaPoint-iAChartWithFunctionsWidget::POINT_RADIUS/2 && lx <= viewXLeftSigmaPoint+iAChartWithFunctionsWidget::POINT_RADIUS/2 &&
		ly >= viewYPoint-iAChartWithFunctionsWidget::POINT_RADIUS/2 && ly <= viewYPoint+iAChartWithFunctionsWidget::POINT_RADIUS/2)
		m_selectedPoint = 1;
	else if (lx >= viewXRightSigmaPoint-iAChartWithFunctionsWidget::POINT_RADIUS/2 && lx <= viewXRightSigmaPoint+iAChartWithFunctionsWidget::POINT_RADIUS/2 &&
			ly >= viewYPoint-iAChartWithFunctionsWidget::POINT_RADIUS/2 && ly <= viewYPoint+iAChartWithFunctionsWidget::POINT_RADIUS/2)
		m_selectedPoint = 2;
	else if (lx >= viewXPoint-iAChartWithFunctionsWidget::POINT_RADIUS && lx <= viewXPoint+iAChartWithFunctionsWidget::POINT_RADIUS &&
			ly >= viewYPoint-iAChartWithFunctionsWidget::POINT_RADIUS && ly <= viewYPoint+iAChartWithFunctionsWidget::POINT_RADIUS)
		m_selectedPoint = 0;

	else
		m_selectedPoint = -1;

	return m_selectedPoint;
}

void iAChartFunctionGaussian::moveSelectedPoint(int x, int y)
{
	y = clamp(0, chart->geometry().height() - chart->bottomMargin() - 1, y);
	if (m_selectedPoint != -1)
	{
		switch(m_selectedPoint)
		{
			case 0:
			{
				x = clamp(0, chart->geometry().width() - 1, x);
				m_mean = v2dX(x);
			}
			break;
			case 1: case 2:
			{
				m_sigma = fabs(m_mean - v2dX(x));
				if (m_sigma <= std::numeric_limits<double>::epsilon())
					m_sigma = fabs(m_mean - v2dX(x+1));
			}
		}

		double meanValue = 1.0/(m_sigma*sqrt(2*vtkMath::Pi()))*chart->yZoom();
		m_multiplier  = (double)y /(chart->geometry().height() - chart->bottomMargin()-1)*chart->yBounds()[1] /meanValue;
	}
}

void iAChartFunctionGaussian::reset()
{}

size_t iAChartFunctionGaussian::numPoints() const
{
	return 3;
}

void iAChartFunctionGaussian::setMultiplier(int multiplier)
{
	double meanValue = 1.0/(m_sigma*sqrt(2*vtkMath::Pi()))*chart->yZoom();
	m_multiplier = v2dY(multiplier) / meanValue;
}

// TODO: unify somewhere!
double iAChartFunctionGaussian::v2dX(int x)
{
	return ((double)(x-chart->xShift()) / (double)chart->geometry().width() * chart->xRange()) /chart->xZoom() + chart->xBounds()[0];
}

double iAChartFunctionGaussian::v2dY(int y)
{
	return chart->yMapper().srcToDst(y) *chart->yBounds()[1] /chart->yZoom();
}

int iAChartFunctionGaussian::d2vX(double x)
{
	return (int)((x - chart->xBounds()[0]) * (double)chart->geometry().width() / chart->xRange()*chart->xZoom()) +chart->xShift();
}

int iAChartFunctionGaussian::d2vY(double y)
{
	return (int)(y /chart->yBounds()[1] *(double)(chart->geometry().height() - chart->bottomMargin()-1) *chart->yZoom());
}

int iAChartFunctionGaussian::d2iX(double x)
{
	return d2vX(x) -chart->xShift();
}

int iAChartFunctionGaussian::d2iY(double y)
{
	return d2vY(y);
}

double iAChartFunctionGaussian::i2dX(int x)
{
	return ((double)x / (double)chart->geometry().width() * chart->xRange()) /chart->xZoom() + chart->xBounds()[0];
}
