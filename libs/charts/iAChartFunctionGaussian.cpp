/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAChartFunctionGaussian.h"

#include "iAChartWithFunctionsWidget.h"
#include "iAMapper.h"
#include "iAMathUtility.h"

#include <vtkMath.h>

#include <QPen>
#include <QPainter>
#include <QMouseEvent>

namespace
{
	double computeGaussValue(double mean, double sigma, double multiplier, double x)
	{
		return 1.0 / (sigma * sqrt(2 * vtkMath::Pi())) *
			exp(-pow((x - mean) / sigma, 2) / 2) * multiplier;
	}
	double const SigmaHandleFactor = 2;
}

iAChartFunctionGaussian::iAChartFunctionGaussian(iAChartWithFunctionsWidget *chart, QColor &color, bool res):
	iAChartFunction(chart),
	m_color(color),
	m_selectedPoint(-1),
	m_mean(0.0),
	m_sigma(0.0),
	m_multiplier(0.0)
{
	if (res)
	{
		reset();
	}
}

void iAChartFunctionGaussian::draw(QPainter &painter)
{
	draw(painter, m_color, LineWidthUnselected);
}

void iAChartFunctionGaussian::draw(QPainter &painter, QColor color, int lineWidth)
{
	bool active = (m_chart->selectedFunction() == this);

	QPen pen = painter.pen();
	pen.setColor(color);
	pen.setWidth(lineWidth);
	painter.setPen(pen);

	double range = m_chart->xRange();
	double startStep = range / 100;
	double step = startStep;

	double dataX1 = m_chart->xBounds()[0];
	double dataX2 = dataX1;
	double dataY1 = computeGaussValue(m_mean, m_sigma, m_multiplier, dataX1);

	double smallStep = std::max(0.05 * m_sigma, m_chart->xMapper().dstToSrc(1) - m_chart->xBounds()[0]);
	while (dataX2 <= m_chart->xBounds()[1]+step)
	{
		double dataY2 = computeGaussValue(m_mean, m_sigma, m_multiplier, dataX2);

		int pixelX1 = m_chart->xMapper().srcToDst(dataX1);
		int pixelY1 = m_chart->yMapper().srcToDst(dataY1);
		int pixelX2 = m_chart->xMapper().srcToDst(dataX2);
		int pixelY2 = m_chart->yMapper().srcToDst(dataY2);

		painter.drawLine(pixelX1, pixelY1, pixelX2, pixelY2);

		// smaller steps close to the mean: TODO - use adaptive step sizes?
		step = std::abs( 0.5*(dataX1+dataX2)+step - m_mean) < (15 * m_sigma) ?
			smallStep : startStep;
		dataX1 = dataX2;
		dataY1 = dataY2;
		dataX2 = dataX1 + step;
	}

	if (!active)
	{
		return;
	}

	// draw point lines
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

	x  = m_chart->xMapper().srcToDst(m_mean);
	lx = m_chart->xMapper().srcToDst(m_mean - SigmaHandleFactor * m_sigma);
	rx = m_chart->xMapper().srcToDst(m_mean + SigmaHandleFactor * m_sigma);
	y = m_chart->yMapper().srcToDst(meanValue);

	painter.drawLine(lx, y, rx, y);

	// draw points
	drawPoint(painter, x, y, true);
	drawPoint(painter, lx, y, false);
	drawPoint(painter, rx, y, false);
}

int iAChartFunctionGaussian::selectPoint(int mouseX, int mouseY)
{
	double meanValue = 1.0/(m_sigma*sqrt(2*vtkMath::Pi()));

	int viewXPoint = m_chart->data2MouseX(m_mean);
	int viewYPoint = m_chart->yMapper().srcToDst(meanValue*m_multiplier);

	int viewXLeftSigmaPoint  = m_chart->data2MouseX(m_mean - SigmaHandleFactor * m_sigma);
	int viewXRightSigmaPoint = m_chart->data2MouseX(m_mean + SigmaHandleFactor * m_sigma);

	if (std::abs(mouseX - viewXLeftSigmaPoint) <= iAChartFunction::PointRadius / 2 &&
		std::abs(mouseY - viewYPoint) <= iAChartFunction::PointRadius / 2)
	{
		m_selectedPoint = 1;
	}
	else if (std::abs(mouseX - viewXRightSigmaPoint) <= iAChartFunction::PointRadius / 2 &&
		std::abs(mouseY - viewYPoint)- iAChartFunction::PointRadius / 2)
	{
		m_selectedPoint = 2;
	}
	else if (std::abs(mouseX - viewXPoint) <= iAChartFunction::PointRadius &&
		std::abs(mouseY - viewYPoint) <= iAChartFunction::PointRadius)
	{
		m_selectedPoint = 0;
	}
	else
	{
		m_selectedPoint = -1;
	}
	return m_selectedPoint;
}

void iAChartFunctionGaussian::moveSelectedPoint(int mouseX, int mouseY)
{
	mouseY = clamp(0, m_chart->chartHeight()-1, mouseY);
	if (m_selectedPoint != -1)
	{
		switch(m_selectedPoint)
		{
			case 0:
			{
				m_mean = clamp(m_chart->xBounds()[0], m_chart->xBounds()[1], m_chart->mouse2DataX(mouseX));
			}
			break;
			case 1:
			case 2:
			{
				m_sigma = fabs(m_mean - m_chart->mouse2DataX(mouseX)) / SigmaHandleFactor;
				if (m_sigma <= std::numeric_limits<double>::epsilon())
				{
					m_sigma = fabs(m_mean - m_chart->mouse2DataX(mouseX+1));
				}
			}
		}
		double maxValue = 1.0/(m_sigma*sqrt(2*vtkMath::Pi()));
		m_multiplier  = m_chart->yMapper().dstToSrc(mouseY) / maxValue;
	}
}

void iAChartFunctionGaussian::reset()
{}

size_t iAChartFunctionGaussian::numPoints() const
{
	return 3;
}

QString iAChartFunctionGaussian::name() const
{
	return QString("Gaussian (mean=%1, sigma=%2)").arg(m_mean).arg(m_sigma);
}
