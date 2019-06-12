#pragma once

#include "ChartDefinitions.h"
#include <QTCharts/qchartview.h>
#include <QTCharts/QChart.h>
#include <QTCharts>
//#include "QTCharts/QChartView"


class QtCharts::QXYSeries;
class QtCharts::QAbstractSeries; 
class QString; 

using namespace QtCharts;
using namespace chartV; 

class iACutstomChartView : public QChartView
{
Q_OBJECT

public:
	//iACutstomChartView(QChart * chart);
	iACutstomChartView(QWidget *parent);
	~iACutstomChartView();


	void prepareAxisX(const QString &title, double min, double max, uint tick_count);
	void prepareAxisY(const QString &title, double min, double max, uint tick_count); 	
	void addSeries(QAbstractSeries *aSerie, const QString *title, bool noLegendSymbol);

	void setAxisLimits(double xmin, double xMax, double yMin, double yMax);

private:
	
	void setAxisMinMax(QValueAxis *axis, double min, double max, bool updateChart = true);
	void prepareAxis(QValueAxis *axis, const QString &title, double min, double max, uint ticks, chartV::axisMode mode);

	QChart *m_chart; 
	QValueAxis *m_axisX;
	QValueAxis *m_axisY;

};

