#include "iACompBarChart.h"

//iA
#include "charts/iAChartWidget.h"
#include "charts/iAHistogramData.h"
#include "charts/iAPlotTypes.h"

#include "mainwindow.h"

iACompBarChart::iACompBarChart(MainWindow* parent) : QDockWidget(parent)
{
	//TODO biuld gui
	//TODO calculate coefficient of variation
	
	setupUi(this);
	this->setFeatures(DockWidgetVerticalTitleBar);

	//testing
	iAChartWidget* chart = new iAChartWidget(nullptr, "Attributes", "Coefficient of variation");
	std::vector<double> histbinlist = std::vector<double>(4, 1.0);
	size_t bins = 4;

	auto histogramData = iAHistogramData::create(histbinlist, bins, iAValueType::Discrete, 1.0, 4.0);
	chart->addPlot(QSharedPointer<iAPlot>(new iABarGraphPlot(histogramData, QColor(70, 70, 70, 255))));
	chart->update();
	chart->setMinimumHeight(80);
	
	layout_barChart->addWidget(chart);
}
