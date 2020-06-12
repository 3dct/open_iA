#pragma once

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

#include "vtkSmartPointer.h"

class MainWindow;
class iACsvDataStorage;
class QVTKOpenGLNativeWidget;
class vtkContextView;

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
};