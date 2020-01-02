#pragma once

#include "ThresholdDefinitions.h"

#include <vector>

enum LineVisOption {
	horizontally,
	vertically,
	horizontal_xy
};

class QPointF;
class QColor; 

namespace QtCharts
{
	class QLineSeries;
	class QScatterSeries;
	class QXYSeries;
}

//Factory for creating the scatter series
class ChartVisHelper
{
public:
	//Create a scatter series from data typ
	static QtCharts::QLineSeries* createLineSeries(const threshold_defs::ParametersRanges& ranges);
	static QtCharts::QScatterSeries* createScatterSeries(const threshold_defs::ParametersRanges& ranges);
	static QtCharts::QScatterSeries* createScatterSeries(const std::vector<double>& vec_x,
		const std::vector<double>& vec_y); 

	static QtCharts::QScatterSeries *createScatterSeries(const std::vector<QPointF> pts, double *pt_size);
//	static QtCharts::QScatterSeries* createScatterSeries(const QPointF &pt, double pt_size, const QColor &color); 

	static QtCharts::QScatterSeries *createScatterSeries(const QPointF& pt, double pt_size, const QColor* color);
	static QtCharts::QLineSeries* createLineSeries(const std::vector<double>& vec_x,
		const std::vector<double>& vec_y);
	
	
	static QtCharts::QLineSeries* createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option);

	//inline static QPointF create(double x_1, double y_1, double, double x_2, double y_1 )
	static QPointF createPoint(double x, double y) {
		return QPointF(x, y);
	}

private:
	static void fillSeries(QtCharts::QXYSeries* aSeries, const std::vector<double> &vec_x, const std::vector<double> &vec_y);
};
