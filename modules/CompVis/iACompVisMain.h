#pragma once

#include "mainwindow.h"

#include "iACsvDataStorage.h"
#include "dlg_VisMainWindow.h"

//QT
#include"qlist.h"

class iAMultidimensionalScaling;
class iACompBarChart;
class iACompHistogramTable;
class iACompBoxPlot;
class iACompCorrelationMap;
class iAHistogramData;
class iACoefficientOfVariation;

class iACompVisMain
{
   public:
	iACompVisMain(MainWindow* mainWin);
	//load the CSV datasets
	void loadData();

	void updateOtherCharts();
	void updateBarChart();
	
   private:
	void initializeMDS();
	void initializeVariationCoefficient();

	dlg_VisMainWindow* m_mainW;
	iACsvDataStorage* m_dataStorage;

	iAMultidimensionalScaling* m_mds;
	iACoefficientOfVariation* m_cofVar;
	iAHistogramData* m_histData;

	iACompBarChart* m_BarChartDockWidget;
	iACompHistogramTable* m_HistogramTableDockWidget;
	iACompBoxPlot* m_BoxPlotDockWidget;
	iACompCorrelationMap* m_CorrelationMapDockWidget;
};