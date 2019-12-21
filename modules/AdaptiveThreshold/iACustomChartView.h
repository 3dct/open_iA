#pragma once

#include "ChartDefinitions.h"

#include <QChartView>

class QString;

namespace QtCharts
{
	class QValueAxis;
}

class iACustomChartView : public QtCharts::QChartView
{
Q_OBJECT

public:
	iACustomChartView(QWidget *parent);
	~iACustomChartView();
	
	void prepareAxisX(const QString &title, double min, double max, uint tick_count);
	void prepareAxisY(const QString &title, double min, double max, uint tick_count); 	
	void addSeries(QtCharts::QAbstractSeries *aSerie, const QString *title, bool noLegendSymbol);
	
	void setAxisLimits(double xmin, double xMax, double yMin, double yMax);

	void setDiagramTitle(const QString& title);

	void setXTite(const QString& title);

	void setYTitle(const QString& title);

private:
	
	void setAxisMinMax(QtCharts::QValueAxis *axis, double min, double max, bool updateChart = true);
	void prepareAxis(QtCharts::QValueAxis *axis, const QString &title, double min, double max, uint ticks, chartV::axisMode mode);

	QtCharts::QChart *m_chart; 
	QtCharts::QValueAxis *m_axisX;
	QtCharts::QValueAxis *m_axisY;
};

