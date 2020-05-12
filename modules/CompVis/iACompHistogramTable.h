#pragma once

//#include "iAPlotData.h"
//CompVis
#include "iACsvDataStorage.h"
//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

class MainWindow;
class iACsvDataStorage;
class QVTKOpenGLNativeWidget;

class iACompHistogramTable : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT
   public:
	iACompHistogramTable(MainWindow* parent, csvDataType::ArrayType* mdsResult);

   private:
	csvDataType::ArrayType* m_mdsMatrix;

	QVTKOpenGLNativeWidget* qvtkWidget;
};