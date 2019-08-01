#pragma once
#include <QtCharts>
#include <vector>

using namespace QtCharts;

class QtCharts::QXYSeries;
class ParametersRanges; 

class ChartVisHelper
{
public:
	//Create a scatter series from data typ
	static QXYSeries* create(const ParametersRanges& ranges); 
	static QXYSeries* create(const std::vector<double> &vec_x, const std::vector<double> &vec_y); 

private:
	static void fillSeries(QXYSeries* aSeries, const std::vector<double> &vec_x, const std::vector<double> &vec_y);
	
	

};

