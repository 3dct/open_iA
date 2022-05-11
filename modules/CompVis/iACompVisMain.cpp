/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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
	result->m_mainW = new dlg_VisMainWindow(result->m_dataStorage->getData(), result->m_mds, mainWin, result);
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
	QVBoxLayout* layout1 = new QVBoxLayout;
	m_mainW->centralwidget->setLayout(layout1);

	//add histogram table
	m_HistogramTableDockWidget = new iACompHistogramTable(m_mainWindow, m_mds, m_dataStorage, this);
	layout1->addWidget(m_HistogramTableDockWidget);

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
	dlg_CSVReader* dlg = new dlg_CSVReader(m_mainWindow);

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
{
	//reinitialize histogram table
	m_HistogramTableDockWidget->reinitializeHistogramTable(m_mds);

	//reinitialize correlation map
	m_CorrelationMapDockWidget->reinitializeCorrelationMap(m_corCoeff);

	//reinitialize bar chart
	m_BarChartDockWidget->reinitializeBarChart(m_cofVar);

	//reinitialize box plot
	m_BoxPlotDockWidget->setOrderedPositions(m_BarChartDockWidget->getOrderedPositions());
	m_BoxPlotDockWidget->reinitializeBoxPlot();
}

/******************************************  Order Methods  **********************************/
void iACompVisMain::orderHistogramTableAscending()
{
	m_HistogramTableDockWidget->drawHistogramTableInAscendingOrder();
}

void iACompVisMain::orderHistogramTableDescending()
{
	m_HistogramTableDockWidget->drawHistogramTableInDescendingOrder();
}

void iACompVisMain::orderHistogramTableAsLoaded()
{
	m_HistogramTableDockWidget->drawHistogramTableInOriginalOrder();
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
	m_HistogramTableDockWidget->showSelectionOfCorrelationMap(dataIndxSelectedType);
}

void iACompVisMain::resetHistogramTableFromCorrelationMap()
{
	m_HistogramTableDockWidget->removeSelectionOfCorrelationMap();
	m_HistogramTableDockWidget->renderWidget();
}