// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompVisMain.h"

//testing
#include "iALog.h"

//CompVis
#include "dlg_CSVReader.h"
#include "dlg_VisMainWindow.h"
#include "iACompBarChart.h"
#include "iACompBoxPlot.h"
#include "iACompCorrelationMap.h"
#include "iACompHistogramTable.h"
#include "iACompHistogramVis.h"
#include "iACsvDataStorage.h"
#include "iACoefficientOfVariation.h"
#include "iACorrelationCoefficient.h"
#include "iAComp3DView.h"
#include "iAMultidimensionalScaling.h"

#include <QBoxLayout>

iACompVisMain::iACompVisMain(iAMainWindow* mainWin) : m_mainWindow(mainWin)
{
}

void iACompVisMain::start(iAMainWindow* mainWin)
{
	auto result = new iACompVisMain(mainWin);
	//load data
	if (!result->loadData())
	{  //quit the program when no data was selected
		return;
	}

	//calculate metrics
	result->initializeMDS();
	result->initializeVariationCoefficient();
	result->initializeCorrelationCoefficient();

	//open iAMainWindow with its dockWidgets
	result->m_mainW = new dlg_VisMainWindow(result->m_dataStorage, result->m_mds, mainWin, result, iACompVisOptions::getComputeMDS());

	if (result->m_mainW->failed())
	{
		result->m_mainW->parent()->deleteLater();
		return;
	}

	result->initGUI();
	// ensure that iACompVisMain is deleted when dlg_VisMainWindow is closed
	QObject::connect(result->m_mainW, &QObject::destroyed, [result] { delete result; });
}

void iACompVisMain::initGUI()
{
	QHBoxLayout* layout3D = new QHBoxLayout;
	m_mainW->centralwidget->setLayout(layout3D);

	//add 3D View
	if (iACompVisOptions::getShow3DViews())
	{
		QGridLayout* gridL = new QGridLayout;
		layout3D->insertLayout(0, gridL);
		m_3DViewDockWidget = new iAComp3DView(m_mainWindow, m_dataStorage);
		m_3DViewDockWidget->constructGridLayout(gridL);
	}

	QVBoxLayout* layout1 = new QVBoxLayout;
	layout3D->insertLayout(0, layout1);

	//add histogram table
	m_HistogramTableDockWidget =
		new iACompHistogramTable(m_mainWindow, m_dataStorage, this, iACompVisOptions::getComputeMDS());
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
	LOG(lvlError,QString("No Dataset was chosen. Therefore the module CompVis closed."));
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

void iACompVisMain::reinitializeCharts(iACsvDataStorage* storage)
{
	//reinitialize histogram table
	m_HistogramTableDockWidget->setDataStorage(storage);
	m_HistogramTableDockWidget->reinitializeHistogramTable();

	//reset all other charts
	resetOtherCharts();
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

void iACompVisMain::enableCurveTable()
{
	m_HistogramTableDockWidget->drawWhiteCurveTable();
}

void iACompVisMain::deactivateOrderingButton()
{
	m_mainW->deactivateOrdering();
}

void iACompVisMain::activateOrderingButton()
{
	m_mainW->activateOrdering();
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

	//3D View
	if (iACompVisOptions::getShow3DViews())
	{
		update3DView(selectedData, pickStatistic);
	}
}

void iACompVisMain::update3DView(
	csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic)
{
	m_3DViewDockWidget->update3DViews(selectedData, pickStatistic);
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

	//3D View
	if (iACompVisOptions:: getShow3DViews())
	{
		reset3DViews();
	}
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

void iACompVisMain::reset3DViews()
{
	m_3DViewDockWidget->reset3DViews();
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
