#include "ChartVisHelper.h"
#include "ThresholdDefinitions.h"
#include <vector>
/*
QLineSeries* ChartVisHelper::createLineSeries(const ParametersRanges& ranges)
{
	QLineSeries* series = new QLineSeries;
	initializeSeries(series);
}
QScatterSeries* ChartVisHelper::createScatterSeries(const ParametersRanges& ranges)
{
	QScatterSeries* series = new QLineSeries;
	initializeSeries(series);
}

ChartVisHelper::initializeSeries(QXYSeries* series)
{
...
}
*/

/*
enum SeriesType
{
	Line,
	Scatter
};

QXYSeries* ChartVisHelper::create(const ParametersRanges& ranges, int option)
{
	QXYSeries* series;
	switch (option) {
		case Line: series = new QLineSeries; break;
		case Scatter: series = new QScatterSeries; break;
	}
	...
*/

QXYSeries* ChartVisHelper::create(const ParametersRanges& ranges)
{
	QXYSeries* series = new QLineSeries; 
	const std::vector<double> x_series = ranges.getXRange(); 
	const std::vector<double> y_series = ranges.getYRange();
	
	if (x_series.empty() || y_series.empty()) {
		return series = nullptr; 
	}

	fillSeries(series, x_series, y_series); 
	return series; 
}

QXYSeries* ChartVisHelper::create(const std::vector<double> &vec_x, const std::vector<double> &vec_y)
{
	QXYSeries* series = new QScatterSeries;
	
	if( !((vec_x.size() > 0 ) && 
		(vec_x.size() == vec_y.size())))
		return series = nullptr; 

	fillSeries(series, vec_x, vec_y); 

	return series;

}

void ChartVisHelper::fillSeries(QXYSeries* aSeries, const std::vector<double> &vec_x, const std::vector<double> &vec_y)
{
	if (!aSeries)
		return; 

	/*if (!((vec_x.size() > 0) &&
		(vec_x.size() == vec_y.size())))
		return; */

	double x = 0;
	double y = 0;

	for (size_t ind = 0; ind < vec_x.size(); ++ind) {
		x = vec_x[ind];
		y = vec_y[ind];
		aSeries->append(x, y);
	}


}
