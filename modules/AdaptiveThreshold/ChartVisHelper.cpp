#include "ChartVisHelper.h"
#include "ThresholdDefinitions.h"
#include <vector>

QLineSeries* ChartVisHelper::createLineSeries(const ParametersRanges& ranges)
{
	QLineSeries * series = new QLineSeries;
	const std::vector<double> x_series = ranges.getXRange(); 
	const std::vector<double> y_series = ranges.getYRange();
	
	if (x_series.empty() || y_series.empty()) {
		return series = nullptr; 
	}

	fillSeries(series, x_series, y_series); 
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

QScatterSeries* ChartVisHelper::createScatterSeries(const ParametersRanges& ranges)
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
