#include "ChartVisHelper.h"
#include "ThresholdDefinitions.h"

#include <iAConsole.h>

#include <QLineSeries>
#include <QScatterSeries>

#include <vector>

QtCharts::QLineSeries* ChartVisHelper::createLineSeries(const threshold_defs::ParametersRanges& ranges)
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

QtCharts::QLineSeries* ChartVisHelper::createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option)
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



QtCharts::QScatterSeries* ChartVisHelper::createScatterSeries(const std::vector<double>& vec_x, const std::vector<double>& vec_y)
{
	if (!((vec_x.size() > 0) && (vec_x.size() == vec_y.size())))
	{
		return nullptr;
	}
	auto series = new QtCharts::QScatterSeries;
	fillSeries(series, vec_x, vec_y);
	return series;

}

QtCharts::QScatterSeries* ChartVisHelper::createScatterSeries(const threshold_defs::ParametersRanges& ranges)
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

QT_CHARTS_NAMESPACE::QScatterSeries* ChartVisHelper::createScatterSeries(const std::vector<QPointF> pts, double* pt_size)
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

QtCharts::QScatterSeries* ChartVisHelper::createScatterSeries(const QPointF& pt, double pt_size, const QColor *color)
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

QtCharts::QLineSeries *ChartVisHelper::createLineSeries(const std::vector<double> &vec_x, const std::vector<double> &vec_y)
{
	if (!((vec_x.size() > 0) && (vec_x.size() == vec_y.size())))
	{
		return nullptr;
	}
	auto series = new QtCharts::QLineSeries;
	fillSeries(series, vec_x, vec_y); 
	return series;
}

void ChartVisHelper::fillSeries(QtCharts::QXYSeries* aSeries, const std::vector<double> &vec_x, const std::vector<double> &vec_y)
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
