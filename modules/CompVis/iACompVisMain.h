#pragma once

#include "iAMainWindow.h"

#include "iACsvDataStorage.h"
#include "dlg_VisMainWindow.h"
#include "iACompHistogramTableData.h"
#include "vtkSmartPointer.h"

//QT
#include"qlist.h"

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
	iACompVisMain(iAMainWindow* mainWin);

	//load the CSV datasets
	//when nothing was loaded it returns false
	bool loadData();

	void reinitializeCharts();
	void reintitalizeMetrics();

	void enableUniformTable();
	void enableBayesianBlocks();
	void enableNaturalBreaks();
	void enableCurveTable();

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
	void initializeMDS();
	void initializeVariationCoefficient();
	void initializeCorrelationCoefficient();

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