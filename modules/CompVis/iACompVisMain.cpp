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

//iA
#include "charts/iAHistogramData.h"

//QT
#include <QMessageBox>
#include <QBoxLayout>

iACompVisMain::iACompVisMain(MainWindow* mainWin)
{
	//load data
	loadData();

	//calculate MDS
	initializeMDS();

	//open mainwindow with its dockWidgets
	m_mainW = new dlg_VisMainWindow(m_data, m_mds, mainWin);

	QVBoxLayout* layout1 = new QVBoxLayout;
	m_mainW->centralwidget->setLayout(layout1);

	//add histogram table
	m_HistogramTableDockWidget = new iACompHistogramTable(mainWin, m_mds);
	layout1->addWidget(m_HistogramTableDockWidget);

	QHBoxLayout* layout2 = new QHBoxLayout;
	layout1->addLayout(layout2);

	//add correlation map
	m_CorrelationMapDockWidget = new iACompCorrelationMap(mainWin);
	layout2->addWidget(m_CorrelationMapDockWidget);

	QVBoxLayout* layout3 = new QVBoxLayout;
	layout2->addLayout(layout3);

	//add bar chart
	m_BarChartDockWidget = new iACompBarChart(mainWin);
	layout3->addWidget(m_BarChartDockWidget);

	//add box plot
	m_BoxPlotDockWidget = new iACompBoxPlot(mainWin);
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

	m_data = dlg->getCsvDataStorage();
}

void iACompVisMain::initializeMDS()
{
	m_mds = new iAMultidimensionalScaling(m_data);

	//todo calculate histogramtable again
}

