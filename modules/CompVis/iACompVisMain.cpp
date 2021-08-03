#include "iACompVisMain.h"

//testing
#include "iALog.h"

//CompVis
#include "dlg_CSVReader.h"
#include "iACompBarChart.h"
#include "iACompBoxPlot.h"
#include "iACompHistogramTable.h"
#include "iACompCorrelationMap.h"

#include "iAMultidimensionalScaling.h"
#include "iACoefficientOfVariation.h"
#include "iACorrelationCoefficient.h"

//QT
#include <QMessageBox>
#include <QBoxLayout>

iACompVisMain::iACompVisMain(iAMainWindow* mainWin):
	m_mainWindow(mainWin)
{
	//load data
	if (!loadData()) 
	{	//quit the program when no data was selected
		return;
	};

	//calculate metrics
	initializeMDS();
	initializeVariationCoefficient();
	initializeCorrelationCoefficient();

	//open iAMainWindow with its dockWidgets
	m_mainW = new dlg_VisMainWindow(m_dataStorage->getData(), m_mds, m_mainWindow, this);

	QVBoxLayout* layout1 = new QVBoxLayout;
	m_mainW->centralwidget->setLayout(layout1);

	//add histogram table
	m_HistogramTableDockWidget = new iACompHistogramTable(mainWin, m_mds, m_dataStorage, this);
	layout1->addWidget(m_HistogramTableDockWidget->getHistogramTableVis());

	QHBoxLayout* layout2 = new QHBoxLayout;
	layout1->addLayout(layout2);

	//add correlation map
	m_CorrelationMapDockWidget = new iACompCorrelationMap(m_mainWindow, m_corCoeff, m_dataStorage, this);
	layout2->addWidget(m_CorrelationMapDockWidget);

	QVBoxLayout* layout3 = new QVBoxLayout;
	layout2->addLayout(layout3);

	//add bar chart
	m_BarChartDockWidget = new iACompBarChart(m_mainWindow, m_cofVar, m_dataStorage);
	layout3->addWidget(m_BarChartDockWidget);

	//add box plot
	m_BoxPlotDockWidget = new iACompBoxPlot(m_mainWindow, m_dataStorage);
	m_BoxPlotDockWidget->setOrderedPositions(m_BarChartDockWidget->getOrderedPositions());
	layout3->addWidget(m_BoxPlotDockWidget);

	m_mainW->show();
}

bool iACompVisMain::loadData()
{
	dlg_CSVReader* dlg = new dlg_CSVReader();

	if (dlg->exec() != QDialog::Accepted)
	{
		noDatasetChosenMessage();
		return false;
	}

	m_dataStorage = dlg->getCsvDataStorage();

	if(m_dataStorage->getDatasetNames()->size() == 0)
	{
		noDatasetChosenMessage();
		return false;
	}

	return true;
}

void iACompVisMain::noDatasetChosenMessage()
{
	LOG(lvlDebug,QString("No Dataset was chosen. Therefore the module CompVis closed."));
}

/******************************************  Initialization Methods  **********************************/
void iACompVisMain::initializeMDS()
{
	m_mds = new iAMultidimensionalScaling(m_dataStorage->getData());
}

void iACompVisMain::initializeVariationCoefficient() 
{
	m_cofVar = new iACoefficientOfVariation(m_dataStorage);
}

void iACompVisMain::initializeCorrelationCoefficient()
{
	m_corCoeff = new iACorrelationCoefficient(m_dataStorage);
}

void iACompVisMain::reintitalizeMetrics()
{
	//calculate metrics
	initializeMDS();
	m_mainW->updateMDS(m_mds);

	initializeVariationCoefficient();
	initializeCorrelationCoefficient();
}

void iACompVisMain::reinitializeCharts()
{ //TODO repair recalculate MDS!
	//reinitialize histogram table
	//m_HistogramTableDockWidget->reinitializeHistogramTable(m_mds);

	//reinitialize correlation map
	//m_CorrelationMapDockWidget->reinitializeCorrelationMap(m_corCoeff);

	//reinitialize bar chart
	//m_BarChartDockWidget->reinitializeBarChart(m_cofVar);

	//reinitialize box plot
//	m_BoxPlotDockWidget->setOrderedPositions(m_BarChartDockWidget->getOrderedPositions());
//	m_BoxPlotDockWidget->reinitializeBoxPlot();
}

/******************************************  Change Table Visualization Methods  **********************************/

void iACompVisMain::enableUniformTable()
{
	m_HistogramTableDockWidget->drawUniformTable();
}

void iACompVisMain::enableBayesianBlocks()
{
	m_HistogramTableDockWidget->drawBayesianBlocksTable();
}

void iACompVisMain::enableNaturalBreaks()
{
	m_HistogramTableDockWidget->drawNaturalBreaksTable();
}

/******************************************  Order Methods  **********************************/
void iACompVisMain::orderHistogramTableAscending()
{
	m_HistogramTableDockWidget->drawDatasetsInAscendingOrder();
}

void iACompVisMain::orderHistogramTableDescending()
{
	m_HistogramTableDockWidget->drawDatasetsInDescendingOrder();
}

void iACompVisMain::orderHistogramTableAsLoaded()
{
	m_HistogramTableDockWidget->drawDatasetsInOriginalOrder();
}

/******************************************  Update Methods  **********************************/

void iACompVisMain::updateOtherCharts(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic)
{
	// Bar Chart
	updateBarChart(selectedData, pickStatistic);
	 
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

void iACompVisMain::updateBarChart(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic)
{
	std::vector<double>* coefficients = m_cofVar->recalculateCoefficentOfVariation(selectedData);
	m_BarChartDockWidget->updateBarChart(coefficients, pickStatistic);
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

void iACompVisMain::updateHistogramTableFromCorrelationMap(std::map<int, double>* dataIndxSelectedType)
{
	m_HistogramTableDockWidget->getHistogramTableVis()->showSelectionOfCorrelationMap(dataIndxSelectedType);
}

void iACompVisMain::resetHistogramTableFromCorrelationMap()
{
	m_HistogramTableDockWidget->getHistogramTableVis()->removeSelectionOfCorrelationMap();
	m_HistogramTableDockWidget->getHistogramTableVis()->renderWidget();
}