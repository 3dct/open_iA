// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAChartVisHelper.h"
#include "iAThresholdDefinitions.h"

#include <QLineSeries>
#include <QXYSeries>

#include <vector>

void fillSeries(QXYSeries* aSeries, const std::vector<double>& vec_x, const std::vector<double>& vec_y)
{
	if (!aSeries)
	{
		throw std::invalid_argument("fillSeries: Empty series given!");
	}
	for (size_t ind = 0; ind < vec_x.size(); ++ind)
	{
		aSeries->append(vec_x[ind], vec_y[ind]);
	}
}

QLineSeries* createLineSeries(const threshold_defs::iAParametersRanges& ranges)
{
	QLineSeries* series = new QLineSeries;
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

QLineSeries* createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option)
{
	double x_1 = pt_1.x();
	double y_1 = pt_1.y();
	double x_2 = pt_2.x();
	double y_2 = pt_2.y();

	//horizontal xy use xy coordinates
	QLineSeries* series = new QLineSeries;
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
