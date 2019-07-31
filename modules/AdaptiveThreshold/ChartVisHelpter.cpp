#include "ChartVisHelpter.h"

QT_CHARTS_NAMESPACE::QXYSeries* ChartVisHelpter::create(const ParametersRanges& ranges)
{
	QXYSeries* series = new QXYSeries; 
	const std::vector<double> x_series = ranges.getXRange(); 
	const std::vector<double> y_series = ranges.getYRange();
	double x = 0; 
	double y = 0; 

	for (size_t ind < x_series.size(); ++i){
		x = x_series[i]; 
		y = y_series[i]; 
		series->append(x, y);
	}

	return series; 
}
