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
class vtkChartXY;


class iACompBarChart : public QDockWidget, public Ui_CompBarChart
{
	Q_OBJECT
   public:
	iACompBarChart(MainWindow* parent, iACoefficientOfVariation* coeffVar, iACsvDataStorage* dataStorage);

	void showEvent(QShowEvent* event);
	void renderWidget();

	//update the bar chart visualization with the selected objects
	void updateBarChart(std::vector<double>* coefficientsOriginal);
	//reset the bar chart for all values of all datasets
	void resetBarChart();

	//get the position of the attributes according to their similiarity (coefficient of variation)
	std::vector<double>* getOrderedPositions();
	std::vector<double>* getSelectedOrderedPositions();

private:

	vtkSmartPointer<vtkDoubleArray> vectorToVtkDataArray(std::vector<double>* input, const char* name);
	//return array with indices of the length of the input vector
	vtkSmartPointer<vtkIntArray> getIndexArray(std::vector<double>* input, const char* name);
	//change interval size from [oldMin,oldMax] to [newMin,newMax]
	std::vector<double>* changeInterval(std::vector<double>* input, double newMax, double newMin, double oldMax, double oldMin);
	//sort the attributes according to their coefficient of variation & save the new position in the vector that is returned
	std::vector<double>* sortWithMemory(std::vector<double>* input);
	//removes the label attribute
	void removeLabelAttribute(std::vector<double>* input, QStringList* names);


	iACsvDataStorage* m_dataStorage;
	iACoefficientOfVariation* m_coeffVar;
	QVTKOpenGLNativeWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;

	vtkSmartPointer<vtkContextView> m_view;

	//stores the coefficient of variation for all values of all datasets
	std::vector<double>* coefficients;
	//stores the names of all attributes
	QStringList* attrNames;
	
	//stores the new positions of all values for all datasets after sorting according to the coefficient of variation
	std::vector<double>* orderedPositions;
	// stores the new positions after sorting according to the coefficient of variation
	std::vector<double>* selected_orderedPositions;
	
	//chart in which the bar chart is stored
	vtkSmartPointer<vtkChartXY> chart;
};