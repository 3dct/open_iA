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
#include "iAChartVisHelper.h"
#include "iAThresholdDefinitions.h"

#include <iAConsole.h>

#include <QLineSeries>
#include <QScatterSeries>

#include <vector>

void fillSeries(QtCharts::QXYSeries* aSeries, const std::vector<double>& vec_x, const std::vector<double>& vec_y)
{
	if (!aSeries)
	{
		DEBUG_LOG("fillSeries: Empty series given!")
			return;
	}
	for (size_t ind = 0; ind < vec_x.size(); ++ind)
	{
		aSeries->append(vec_x[ind], vec_y[ind]);
	}
}

QtCharts::QLineSeries* createLineSeries(const threshold_defs::iAParametersRanges& ranges)
{
	QtCharts::QLineSeries * series = new QtCharts::QLineSeries;
	const std::vector<double> x_series = ranges.getXRange();
	const std::vector<double> y_series = ranges.getYRange();

	if (x_series.empty() || y_series.empty())
	{
		delete series;
		throw std::invalid_argument("data is empty");
	}

	fillSeries(series, x_series, y_series);
	return series;
}

QtCharts::QLineSeries* createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option)
{
	double x_1 = pt_1.x();
	double y_1 = pt_1.y();
	double x_2 = pt_2.x();
	double y_2 = pt_2.y();

	//horizontal xy use xy coordinates
	QtCharts::QLineSeries* series = new QtCharts::QLineSeries;
	switch (option) {
	case horizontally: y_1 = 0; /*y_2 = 0*/;break;
	case vertically: /*x_1 = 0*/; x_2 = 0; break;
	case horizontal_xy: break;
	//leave it
	default:
		break;
	}

	series->append(x_1, y_1);
	series->append(x_2, y_2);

	return series;
}

QtCharts::QScatterSeries* createScatterSeries(const std::vector<double>& vec_x, const std::vector<double>& vec_y)
{
	if (!((vec_x.size() > 0) && (vec_x.size() == vec_y.size())))
	{
		return nullptr;
	}
	auto series = new QtCharts::QScatterSeries;
	fillSeries(series, vec_x, vec_y);
	return series;

}

QtCharts::QScatterSeries* createScatterSeries(const threshold_defs::iAParametersRanges& ranges)
{
	const std::vector<double> x_series = ranges.getXRange();
	const std::vector<double> y_series = ranges.getYRange();

	if (x_series.empty() || y_series.empty())
	{
		return nullptr;
	}

	auto series = new QtCharts::QScatterSeries;
	fillSeries(series, x_series, y_series);
	return series;

}

QtCharts::QScatterSeries* createScatterSeries(const std::vector<QPointF> pts, double* pt_size)
{
	if (pts.empty())
	{
		DEBUG_LOG("createScatterSeries: Empty points given!")
		return nullptr;
	}

	auto series = new QtCharts::QScatterSeries;
	for (const QPointF& el : pts)
	{
		series->append(el);
	}
	if (pt_size)
	{
		if (*pt_size > 0)
		{
			series->setMarkerSize(*pt_size);
		}
	}
	return series;
}

QtCharts::QScatterSeries* createScatterSeries(const QPointF& pt, double pt_size, const QColor *color)
{
	if (pt.isNull())
	{
		DEBUG_LOG("createScatterSeries: null pt given!")
		return nullptr;
	}
	auto series = new QtCharts::QScatterSeries;
	series->append(pt);
	series->setMarkerShape(QtCharts::QScatterSeries::MarkerShapeRectangle);
	if (color)
	{
		series->setColor(*color);
	}

	if (pt_size > 5.0) series->setMarkerSize(pt_size);
	return series;

}

QtCharts::QLineSeries* createLineSeries(const std::vector<double> &vec_x, const std::vector<double> &vec_y)
{
	if (!((vec_x.size() > 0) && (vec_x.size() == vec_y.size())))
	{
		return nullptr;
	}
	auto series = new QtCharts::QLineSeries;
	fillSeries(series, vec_x, vec_y);
	return series;
}
