#pragma once

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

#include "vtkSmartPointer.h"
#include "iACsvDataStorage.h"

#include <vtkChartBox.h>

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
	//table containing all values
	vtkSmartPointer<vtkTable> inputBoxPlotTable;
	//stores the minimum, first quartile, median, third quartile and maximum of the real values
	vtkSmartPointer<vtkTable> outTable;
	
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

			void setOuterClass(iACompBoxPlot* outerClass);

	protected:
		BoxPlotChart();

	private:
		iACompBoxPlot* m_outerClass;

	};

};


