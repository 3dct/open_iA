#pragma once

//Qt
#include "ui_CompBarChart.h"
#include <QDockWidget>

//vtk
#include "vtkSmartPointer.h"

class MainWindow;
class iACoefficientOfVariation;
class iACsvDataStorage;
class QVTKOpenGLNativeWidget;

class vtkDataArray;
class vtkDoubleArray;
class vtkRenderer;

class vtkIntArray;

class vtkContextView;


class iACompBarChart : public QDockWidget, public Ui_CompBarChart
{
	Q_OBJECT
   public:
	iACompBarChart(MainWindow* parent, iACoefficientOfVariation* coeffVar, iACsvDataStorage* dataStorage);

	void showEvent(QShowEvent* event);
	void renderWidget();

	void updateBarChart(std::vector<double>* coefficients);

private:
	vtkSmartPointer<vtkDoubleArray> vectorToVtkDataArray(std::vector<double>* input, const char* name);
	//return array with indices of the length of the input vector
	vtkSmartPointer<vtkIntArray> getIndexArray(std::vector<double>* input, const char* name);
	std::vector<double>* changeInterval(std::vector<double>* input, double newMax, double newMin, double oldMax, double oldMin);
	std::vector<double>* sortWithMemory(std::vector<double>* input);
	//removes the label attribute
	void removeLabelAttribute(std::vector<double>* input, QStringList* names);


	iACsvDataStorage* m_dataStorage;
	iACoefficientOfVariation* m_coeffVar;
	QVTKOpenGLNativeWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;

	vtkSmartPointer<vtkContextView> m_view;
};