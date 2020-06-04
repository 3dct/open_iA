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

class iACompVisMain
{
   public:
	iACompVisMain(MainWindow* mainWin);
	//load the CSV datasets
	void loadData();
	
   private:
	void initializeMDS();

	dlg_VisMainWindow* m_mainW;
	iACsvDataStorage* m_dataStorage;

	iAMultidimensionalScaling* m_mds;
	iAHistogramData* m_histData;

	iACompBarChart* m_BarChartDockWidget;
	iACompHistogramTable* m_HistogramTableDockWidget;
	iACompBoxPlot* m_BoxPlotDockWidget;
	iACompCorrelationMap* m_CorrelationMapDockWidget;
};