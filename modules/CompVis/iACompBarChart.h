// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACompVisOptions.h"
#include "ui_CompBarChart.h"

//Qt
#include <QDockWidget>

//vtk
#include <vtkContextInteractorStyle.h>
#include <vtkSmartPointer.h>

//CompVis
class iAMainWindow;
class iACoefficientOfVariation;
class iACsvDataStorage;
class iAQVTKWidget;

//vtk
class vtkDataArray;
class vtkDoubleArray;
class vtkRenderer;
class vtkIntArray;
class vtkContextView;
class vtkContextArea;
class vtkUnsignedCharArray;
class vtkPoints;
class vtkFloatArray;
class vtkPropItem;
class vtkTooltipItem;
class vtkPointPicker;
class vtkCellPicker;
class vtkSelectEnclosedPoints;
class vtkPointLocator;
class vtkPolyData;

class iACompBarChart : public QDockWidget, public Ui_CompBarChart
{
	Q_OBJECT
   public:
	iACompBarChart(iAMainWindow* parent, iACoefficientOfVariation* coeffVar, iACsvDataStorage* dataStorage);

	void showEvent(QShowEvent* event);
	void renderWidget();

	//update the bar chart visualization with the selected objects
	void updateBarChart(std::vector<double>* coefficientsOriginal, std::map<int, std::vector<double>>* pickStatistic);
	//reset the bar chart for all values of all datasets
	void resetBarChart();

	//get the position of the attributes according to their similiarity (coefficient of variation)
	std::vector<double>* getOrderedPositions();
	//get the position of the attributes according to their similiarity of the selected objects
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
	void removeLabelAttribute(std::vector<double>* input);

	//creates the original bar chart
	void initializeBarChart();
	//initializes the axes description & ticks
	void initializeAxes(std::vector<double>* orderedPos);
	//adds all bars to the drawing area
	vtkSmartPointer<vtkPropItem> addBars(vtkSmartPointer<vtkPolyData> data, std::string colorArrayName, std::string scaleArrayName, double opacity, double col[3]);
	//redraw the data of the original bar chart but with positions according to the selected bar chart
	void updateOriginalBarChart();
	//update the label position according to the selected bar chart
	void updateLabels();
	//reset the label position according to the original bar chart
	void resetLabels();

	iACsvDataStorage* m_dataStorage;
	//class where the calculation of the coefficient of variation happens
	iACoefficientOfVariation* m_coeffVar;
	iAQVTKWidget* m_qvtkWidget;

	//drawing area of the bars
	vtkSmartPointer<vtkContextArea> m_area;

	//context view for 2D charts
	vtkSmartPointer<vtkContextView> m_view;

	//stores the new positions of all values for all datasets after sorting according to the coefficient of variation
	std::vector<double>* orderedPositions;
	// stores the new positions after sorting according to the coefficient of variation
	std::vector<double>* selected_orderedPositions;

	//stores the names of all attributes
	QStringList* attrNames;

	//stores the coefficient of variation for all values of all datasets
	std::vector<double>* coefficients;
	//stores the coefficient of variation for all values of all datasets in original order
	//according to the order of the attributes in the csv
	std::vector<double>* coefficientsUnordered;
	
	//width of the original bars --> not 1, otherwise there would be no space between them
	const double m_barWidth = 0.8;
	

	vtkSmartPointer<vtkPropItem> m_originalBarChart;
	vtkSmartPointer<vtkPropItem> m_originalBarChartRepositioned;
	vtkSmartPointer<vtkPropItem> m_selectedBarChart;

	//stores the last interaction that was performed to make a reinitialization after minimizing etc. possible
	iACompVisOptions::lastState m_lastState;
	
	//inner class
	class BarChartInteractorStyle : public vtkContextInteractorStyle
	{
		public:
			static BarChartInteractorStyle* New();
			vtkTypeMacro(BarChartInteractorStyle, vtkContextInteractorStyle);

			virtual void OnLeftButtonDown() override;

			virtual void OnLeftButtonUp() override;
			virtual void OnMouseMove() override;
			virtual void OnMiddleButtonDown() override;
			virtual void OnRightButtonDown() override;
			virtual void OnMouseWheelForward() override;
			virtual void OnMouseWheelBackward() override;
			virtual void OnKeyPress() override;
			virtual void OnKeyRelease() override;

			void setTooltip(vtkTooltipItem* tooltip);
			vtkTooltipItem* GetTooltip();
			void hideTooltip();

			void setOuterClass(iACompBarChart* outerClass);

			// build point locator to find the picked bar in the original bar chart
			void buildPointLocatorOriginal(vtkSmartPointer<vtkPolyData> data);
			// build point locator to find the picked bar in the selected bar chart
			void buildPointLocatorSelected(vtkSmartPointer<vtkPolyData> data);
			//resets the point locator and ends the buildlocator for the selected and the repositioned original bar charts
			void resetPointLocatorSelected();
			// build point locator to find the picked bar in the repositioned original bar chart (when selected bar chart is used)
			void buildPointLocatorOriginalRepositioned(vtkSmartPointer<vtkPolyData> data);

			void initializeBarChartInteractorStyle();

		protected:
			BarChartInteractorStyle();

		private:

			void showToolTip(std::vector<double> positions, double* posInScene);
			void setTooltipTextSelectedBarChart(std::vector<double> valuesSelected, std::vector<double> valuesOriginal);
			void setTooltipTextOriginalBarChart(std::vector<double> values);

			iACompBarChart* m_outerClass;
			vtkSmartPointer<vtkTooltipItem> m_toolTip;
			vtkSmartPointer<vtkPointPicker> m_picker;
			//stores the points of the bars of the original bar chart
			vtkSmartPointer<vtkPointLocator> pointLocatorOriginal;
			//stores the points of the bars of the selected bar chart
			vtkSmartPointer<vtkPointLocator> pointLocatorSelected;

			//stores for each bar index of the original bar chart its minX,maxX,minY,maxY
			std::map<int, std::vector<double>>* m_barIndexPositionPairOriginal;
			//stores for each bar index of the selected bar chart its minX,maxX,minY,maxY
			std::map<int, std::vector<double>>* m_barIndexPositionPairSelected;
			//is true when the pointLocatorSelected is empty --> so currently only the original bar chart is shown

			std::map<int, std::vector<double>>* m_barIndexPositionPairOriginalRepositioned;
			bool m_selectedPointLocatorEmpty;
	};

	vtkSmartPointer<BarChartInteractorStyle> style;

};
