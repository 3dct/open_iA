#pragma once
#include <QtCharts>
#include <vector>
#include "ThresholdDefinitions.h"

using namespace QtCharts;

class QtCharts::QXYSeries;
 //class ParametersRanges; 
enum LineVisOption {
	horizontally,
	vertically,
	horizontal_xy
};



class QPointF;


//Factory for creating the scatter series
class ChartVisHelper
{
public:
	//Create a scatter series from data typ
	static QLineSeries* createLineSeries(const threshold_defs::ParametersRanges& ranges);
	static QScatterSeries* createScatterSeries(const threshold_defs::ParametersRanges& ranges);
	static QScatterSeries* createScatterSeries(const std::vector<double>& vec_x,
		const std::vector<double>& vec_y); 
	static QLineSeries* createLineSeries(const std::vector<double>& vec_x, 
		const std::vector<double>& vec_y);
	
	
	static QLineSeries* createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option);

	//inline static QPointF create(double x_1, double y_1, double, double x_2, double y_1 )
	static QPointF createPoint(double x, double y) {
		return QPointF(x, y);
	}

private:
	static void fillSeries(QXYSeries* aSeries, const std::vector<double> &vec_x, const std::vector<double> &vec_y);
	
	

};

