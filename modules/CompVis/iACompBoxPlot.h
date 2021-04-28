#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "iACompVisOptions.h"

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

//vtk
#include <vtkChartBox.h>
#include "vtkPlotBox.h"
#include "vtkSmartPointer.h"

//CompVis
class iAMainWindow;

//vtk
class QVTKOpenGLNativeWidget;
class vtkContextView;
class vtkTable;
class vtkTextActor;
class vtkLookupTable;
class vtkRenderer;

//TODO change to ui_boxplot
class iACompBoxPlot : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT
   public:
	iACompBoxPlot(iAMainWindow* parent, iACsvDataStorage* dataStorage);
	
	void showEvent(QShowEvent* event);
	
	void renderWidget();
	
	void updateLegend();
	
	void setOrderedPositions(std::vector<double>* orderedPositions);

	void updateBoxPlot(csvDataType::ArrayType* selectedData, std::vector<double>* selected_orderedPositions);
	void resetBoxPlot();

private:

	//calculate the median of the given vector iterator
	double median(std::vector<double>::const_iterator begin, std::vector<double>::const_iterator end);
	//calcuate quartiles of the vector v
	std::vector<double>* calcualteQuartiles(std::vector<double> v);
	//calcuate quartiles of the table with VTK
	vtkSmartPointer<vtkTable> calcualteVTKQuartiles(vtkSmartPointer<vtkTable> input);
	//bring the values of the input table in the interval [0,1]
	vtkSmartPointer<vtkTable> normalizeTable(vtkSmartPointer<vtkTable> input);
	vtkSmartPointer<vtkTable> normalizeTableSelected(vtkSmartPointer<vtkTable> input, std::vector<double>* selected_orderedPositions);

	//inner class
	class BoxPlotChart : public vtkChartBox
	{
	public:
		static BoxPlotChart* New();
		vtkTypeMacro(BoxPlotChart, vtkChartBox);

		virtual void SetTooltipInfo(const vtkContextMouseEvent& mouse,
			const vtkVector2d &plotPos,
			vtkIdType seriesIndex, vtkPlot* plot,
			vtkIdType segmentIndex);

		void Update();

		void setOuterClass(iACompBoxPlot* outerClass);

	protected:
		BoxPlotChart();

	private:
		iACompBoxPlot* m_outerClass;
	};

	//inner class 
	class BoxPlot : public vtkPlotBox
	{
	public:
		static BoxPlot* New();
		vtkTypeMacro(BoxPlot, vtkPlotBox);

		virtual bool Paint(vtkContext2D *painter);
		virtual void DrawBoxPlot(int i, unsigned char* rgba, double x, vtkContext2D* painter);

		void setOuterClass(iACompBoxPlot* outerClass);
		void setNumberOfColumns(int nmbCols);

	protected:
		BoxPlot();

	private:
		iACompBoxPlot* m_outerClass;

		int m_numberOfColumns;

	};

	void initializeChart();

	void initializeData();
	void initializeAxes(vtkSmartPointer<BoxPlotChart> chart, bool axesVisibleOn);
	void initializeLutForOriginalBoxPlot();
	void initializeLutForSelectedBoxPlot();
	void initializeLegend(vtkSmartPointer<BoxPlotChart> chart);
	void reorderOriginalData(std::vector<double>* selected_orderedPositions);
	void reorderLegend(std::vector<double>* selected_orderedPositions);

	void drawNotEnoughObjectsSelectedMessage();
	void removeSelectedMessage();

	iACsvDataStorage* m_dataStorage;

	QVTKOpenGLNativeWidget* m_qvtkWidget;
	vtkSmartPointer<vtkContextView> m_view;

	std::vector<double>* maxValsAttr;
	std::vector<double>* minValsAttr;

	//table containing all values
	//vtkSmartPointer<vtkTable> m_originalBoxPlotTable;
	//stores the minimum, first quartile, median, third quartile and maximum of the real values
	vtkSmartPointer<vtkTable> outTable;
	//stores the values of outTable in the interval [0,1]
	vtkSmartPointer<vtkTable> normalizedTable;

	vtkSmartPointer<vtkTable> reorderedNormalizedTable;

	vtkSmartPointer<vtkTable> currentQuartileTable;

	//stores the normalized values in the original order
	vtkSmartPointer<vtkTable> m_originalOrderTable;
	vtkSmartPointer<vtkTable> m_originalOrderTableNotNormalized;

	//new positions calculated from bar chart
	std::vector<double>* m_orderedPositions;

	int m_numberOfAttr;
	std::vector<vtkSmartPointer<vtkTextActor>>* m_legendAttributes;
	vtkSmartPointer<vtkStringArray> labels;

	vtkSmartPointer<BoxPlotChart> m_chartOriginal;
	vtkSmartPointer<BoxPlot> m_boxOriginal;
	vtkSmartPointer<vtkLookupTable> lutOriginal;
	bool finishedInitalization;

	vtkSmartPointer<BoxPlotChart> m_chartSelected;
	vtkSmartPointer<BoxPlot> m_boxSelected;
	vtkSmartPointer<vtkLookupTable> lutSelected;

	vtkSmartPointer<vtkTextActor> notEnoughElementsSelectedTextActor;

	//stores the last interaction that was performed to make a reinitialization after minimizing etc. possible
	iACompVisOptions::lastState m_lastState;
};


