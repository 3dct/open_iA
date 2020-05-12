#pragma once

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

class MainWindow;

//TODO change to ui_correlationmap!!!
class iACompCorrelationMap : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT
   public:
	iACompCorrelationMap(MainWindow* parent);
};

