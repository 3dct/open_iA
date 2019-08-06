#include "ChartVisHelper.h"
#include "ThresholdDefinitions.h"
#include <vector>

QLineSeries* ChartVisHelper::createLineSeries(const threshold_defs::ParametersRanges& ranges)
{
	QLineSeries * series = new QLineSeries;
	const std::vector<double> x_series = ranges.getXRange(); 
	const std::vector<double> y_series = ranges.getYRange();
	
	if (x_series.empty() || y_series.empty()) {
		delete series; 
		throw std::invalid_argument("data is empty"); 
	}

	fillSeries(series, x_series, y_series); 
	return series; 
}

QLineSeries* ChartVisHelper::createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option)
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



QScatterSeries* ChartVisHelper::createScatterSeries(const std::vector<double>& vec_x, const std::vector<double>& vec_y)
{
	QScatterSeries* series = nullptr; 
	if (!((vec_x.size() > 0) &&
		(vec_x.size() == vec_y.size())))
		return series;
	series = new QScatterSeries;
	fillSeries(series, vec_x, vec_y);
	return series;

}

QScatterSeries* ChartVisHelper::createScatterSeries(const threshold_defs::ParametersRanges& ranges)
{
	QScatterSeries* series = new QScatterSeries;
	const std::vector<double> x_series = ranges.getXRange();
	const std::vector<double> y_series = ranges.getYRange();

	if (x_series.empty() || y_series.empty()) {
		return series = nullptr;
	}

	fillSeries(series, x_series, y_series);
	return series;

}

QLineSeries *ChartVisHelper::createLineSeries(const std::vector<double> &vec_x, const std::vector<double> &vec_y)
{
	QLineSeries *series = nullptr;
	if (!((vec_x.size() > 0) &&
		(vec_x.size() == vec_y.size())))
		return series;

	series = new QLineSeries; 
	fillSeries(series, vec_x, vec_y); 
	return series;
}

void ChartVisHelper::fillSeries(QXYSeries* aSeries, const std::vector<double> &vec_x, const std::vector<double> &vec_y)
{
	if (!aSeries)
		return; 

	double x = 0;
	double y = 0;

	for (size_t ind = 0; ind < vec_x.size(); ++ind) {
		x = vec_x[ind];
		y = vec_y[ind];
		aSeries->append(x, y);
	}


}
