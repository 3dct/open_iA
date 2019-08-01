#pragma once
#include <QtCharts>
#include <vector>

using namespace QtCharts;

class QtCharts::QXYSeries;
class ParametersRanges; 


//Factory for creating the scatter series
class ChartVisHelper
{
public:
	//Create a scatter series from data typ
	static QLineSeries* createLineSeries(const ParametersRanges& ranges);
	static QScatterSeries* createScatterSeries(const ParametersRanges& ranges);
	static QScatterSeries* createScatterSeries(const std::vector<double>& vec_x,
		const std::vector<double>& vec_y); 
	static QLineSeries* createLineSeries(const std::vector<double>& vec_x, 
		const std::vector<double>& vec_y);
	
	
private:
	static void fillSeries(QXYSeries* aSeries, const std::vector<double> &vec_x, const std::vector<double> &vec_y);
	
	

};

