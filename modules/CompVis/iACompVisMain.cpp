#include "iACompVisMain.h"

//testing
#include "iAConsole.h"

//CompVis
#include "dlg_CSVReader.h"
#include "iACompBarChart.h"
#include "iACompBoxPlot.h"
#include "iACompHistogramTable.h"
#include "iACompCorrelationMap.h"

#include "iAMultidimensionalScaling.h"
#include "iACoefficientOfVariation.h"
#include "iACorrelationCoefficient.h"

//iA
#include "charts/iAHistogramData.h"

//QT
#include <QMessageBox>
#include <QBoxLayout>

iACompVisMain::iACompVisMain(MainWindow* mainWin)
{
	//load data
	loadData();

	//calculate metrics
	initializeMDS();
	initializeVariationCoefficient();
	initializeCorrelationCoefficient();

	//open mainwindow with its dockWidgets
	m_mainW = new dlg_VisMainWindow(m_dataStorage->getData(), m_mds, mainWin, this);

	QVBoxLayout* layout1 = new QVBoxLayout;
	m_mainW->centralwidget->setLayout(layout1);

	//add histogram table
	m_HistogramTableDockWidget = new iACompHistogramTable(mainWin, m_mds, m_dataStorage, this);
	layout1->addWidget(m_HistogramTableDockWidget);

	QHBoxLayout* layout2 = new QHBoxLayout;
	layout1->addLayout(layout2);

	//add correlation map
	m_CorrelationMapDockWidget = new iACompCorrelationMap(mainWin, m_corCoeff, m_dataStorage);
	layout2->addWidget(m_CorrelationMapDockWidget);

	QVBoxLayout* layout3 = new QVBoxLayout;
	layout2->addLayout(layout3);

	//add bar chart
	m_BarChartDockWidget = new iACompBarChart(mainWin, m_cofVar, m_dataStorage);
	layout3->addWidget(m_BarChartDockWidget);

	//add box plot
	m_BoxPlotDockWidget = new iACompBoxPlot(mainWin, m_dataStorage);
	m_BoxPlotDockWidget->setOrderedPositions(m_BarChartDockWidget->getOrderedPositions());
	layout3->addWidget(m_BoxPlotDockWidget);

	m_mainW->show();
}

void iACompVisMain::loadData()
{
	dlg_CSVReader* dlg = new dlg_CSVReader();

	if (dlg->exec() != QDialog::Accepted)
	{
		return;
	}

	m_dataStorage = dlg->getCsvDataStorage();
}

/******************************************  Initialization Methods  **********************************/
void iACompVisMain::initializeMDS()
{
	m_mds = new iAMultidimensionalScaling(m_dataStorage->getData());

	//todo calculate histogramtable again
}

void iACompVisMain::initializeVariationCoefficient() 
{
	m_cofVar = new iACoefficientOfVariation(m_dataStorage);
}

void iACompVisMain::initializeCorrelationCoefficient()
{
	m_corCoeff = new iACorrelationCoefficient(m_dataStorage);
}

/******************************************  Order Methods  **********************************/
void iACompVisMain::orderHistogramTableAscending()
{
	m_HistogramTableDockWidget->drawHistogramTableInAscendingOrder(m_HistogramTableDockWidget->getBins());
}

void iACompVisMain::orderHistogramTableDescending()
{
	m_HistogramTableDockWidget->drawHistogramTableInDescendingOrder(m_HistogramTableDockWidget->getBins());
}

void iACompVisMain::orderHistogramTableAsLoaded()
{
	m_HistogramTableDockWidget->drawHistogramTableInOriginalOrder(m_HistogramTableDockWidget->getBins());
}

/******************************************  Update Methods  **********************************/

void iACompVisMain::updateOtherCharts(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic)
{
	// Bar Chart
	updateBarChart(selectedData);
	
	//Box Plot
	updateBoxPlot(selectedData);

	//Choropleth Map
	updateCorrelationMap(selectedData, pickStatistic);
}

void iACompVisMain::updateCorrelationMap(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic)
{
	auto coefficients = m_corCoeff->calculateCorrelationCoefficients(selectedData);
	m_CorrelationMapDockWidget->updateCorrelationMap(coefficients, pickStatistic);
}

void iACompVisMain::updateBarChart(csvDataType::ArrayType* selectedData)
{
	std::vector<double>* coefficients = m_cofVar->recalculateCoefficentOfVariation(selectedData);
	m_BarChartDockWidget->updateBarChart(coefficients);
}

void iACompVisMain::updateBoxPlot(csvDataType::ArrayType* selectedData)
{
	m_BoxPlotDockWidget->updateBoxPlot(selectedData, m_BarChartDockWidget->getSelectedOrderedPositions());
}

void iACompVisMain::resetOtherCharts()
{
	resetBarChart();
	resetBoxPlot(); 
	resetCorrelationMap();
}

void iACompVisMain::resetBarChart()
{
	m_BarChartDockWidget->resetBarChart();
}

void iACompVisMain::resetBoxPlot()
{
	m_BoxPlotDockWidget->resetBoxPlot();
}

void iACompVisMain::resetCorrelationMap()
{
	m_CorrelationMapDockWidget->resetCorrelationMap();
}
