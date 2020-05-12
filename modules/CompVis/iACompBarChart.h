#pragma once

//Qt
#include "ui_CompBarChart.h"
#include <QDockWidget>

class iAPlotTypes;
class MainWindow;

class iACompBarChart : public QDockWidget, public Ui_CompBarChart
{
	Q_OBJECT
   public:
	iACompBarChart(MainWindow* parent);
};