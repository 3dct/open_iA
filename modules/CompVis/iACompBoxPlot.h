#pragma once

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

#include "vtkSmartPointer.h"
#include "iACsvDataStorage.h"

class MainWindow;
class QVTKOpenGLNativeWidget;
class vtkContextView;
class vtkTable;

class vtkRenderer;

//TODO change to ui_boxplot
class iACompBoxPlot : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT
   public:
	iACompBoxPlot(MainWindow* parent, iACsvDataStorage* dataStorage);
	void showEvent(QShowEvent* event);
	void renderWidget();

private:

	iACsvDataStorage* m_dataStorage;

	QVTKOpenGLNativeWidget* m_qvtkWidget;
	vtkSmartPointer<vtkContextView> m_view;

	std::vector<double>* maxValsAttr;
	std::vector<double>* minValsAttr;
	vtkSmartPointer<vtkTable> inputBoxPlotTable;

	vtkSmartPointer<vtkRenderer> legendRenderer;
};

