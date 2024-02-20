// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACsvDataStorage.h"

#include <map>

class dlg_VisMainWindow;
class iACoefficientOfVariation;
class iAComp3DView;
class iACompBarChart;
class iACompHistogramTable;
class iACompBoxPlot;
class iACompCorrelationMap;
class iACorrelationCoefficient;
class iAHistogramData;
class iAMultidimensionalScaling;

class iAMainWindow;

class iACompVisMain
{
public:
	static void start(iAMainWindow* mainWin);

	//load the CSV datasets
	//when nothing was loaded it returns false
	bool loadData();

	//reinitalize all charts after the MDS is recomputed
	void reinitializeCharts(iACsvDataStorage* storage);

	void enableUniformTable();
	void enableBayesianBlocks();
	void enableNaturalBreaks();
	void enableCurveTable();

	void orderHistogramTableAscending();
	void orderHistogramTableDescending();
	void orderHistogramTableAsLoaded();
	//deactivates the ordering button in the menu
	void deactivateOrderingButton();
	//activates the ordering button in the menu
	void activateOrderingButton();

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
	void update3DView(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic);

	void resetOtherCharts();
	void resetBarChart();
	void resetBoxPlot();
	void resetCorrelationMap();
	void reset3DViews();

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
	iAComp3DView* m_3DViewDockWidget;
};
