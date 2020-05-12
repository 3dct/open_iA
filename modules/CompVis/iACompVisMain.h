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

class iACompVisMain
{
   public:
	iACompVisMain(MainWindow* mainWin);
	//load the CSV datasets
	void loadData();
	
   private:
	void initializeMDS();

	dlg_VisMainWindow* m_mainW;
	QList<csvFileData>* m_data;
	iAMultidimensionalScaling* m_mds;

	iACompBarChart* m_BarChartDockWidget;
	iACompHistogramTable* m_HistogramTableDockWidget;
	iACompBoxPlot* m_BoxPlotDockWidget;
	iACompCorrelationMap* m_CorrelationMapDockWidget;
};