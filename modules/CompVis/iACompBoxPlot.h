#pragma once

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

class MainWindow;

//TODO change to ui_boxplot
class iACompBoxPlot : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT
   public:
	iACompBoxPlot(MainWindow* parent);
};