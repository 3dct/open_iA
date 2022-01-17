/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "iAMainWindow.h"

#include "iACsvDataStorage.h"
#include "dlg_VisMainWindow.h"
//#include "iACompHistogramTableData.h"

class iAMultidimensionalScaling;
class iACoefficientOfVariation;
class iACorrelationCoefficient;

class iACompBarChart;
class iACompHistogramTable;
class iACompBoxPlot;
class iACompCorrelationMap;
class iAHistogramData;


class iACompVisMain
{
public:
	static void start(iAMainWindow* mainWin);
	//load the CSV datasets
	//when nothing was loaded it returns false
	bool loadData();

	void reinitializeCharts();
	void reintitalizeMetrics();

	void orderHistogramTableAscending();
	void orderHistogramTableDescending();
	void orderHistogramTableAsLoaded();

	//update all charts according to the Histogram Table selection
	//zoomedRowData stores bin data of selected rows that will be zoomed.
	//Each entry in the list represents a row, where any cell(or several) were selected.
	//The first entry is the most upper row that was selected, the ordering is then descending.
	//each entry has as many bins as cells were selected for this row
	void updateOtherCharts(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic);
	//update BarChart  according to the Histogram Table selection
	void updateBarChart(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic);
	void updateBoxPlot(csvDataType::ArrayType* selectedData);
	void updateCorrelationMap(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic);

	void resetOtherCharts();
	void resetBarChart();
	void resetBoxPlot();
	void resetCorrelationMap();

	void updateHistogramTableFromCorrelationMap(std::map<int, double>* dataIndxSelectedType);
	void resetHistogramTableFromCorrelationMap();

private:
	iACompVisMain(iAMainWindow* mainWin);
	void initializeMDS();
	void initializeVariationCoefficient();
	void initializeCorrelationCoefficient();
	void initGUI();

	void noDatasetChosenMessage();


	dlg_VisMainWindow* m_mainW;
	iAMainWindow* m_mainWindow;
	iACsvDataStorage* m_dataStorage;

	iAMultidimensionalScaling* m_mds;
	iACoefficientOfVariation* m_cofVar;
	iACorrelationCoefficient* m_corCoeff;
	
	iACompBarChart* m_BarChartDockWidget;
	iACompHistogramTable* m_HistogramTableDockWidget;
	iACompBoxPlot* m_BoxPlotDockWidget;
	iACompCorrelationMap* m_CorrelationMapDockWidget;
};