#include "iACompBoxPlot.h"

//CompVis
#include "iACsvDataStorage.h"

//iA
#include "mainwindow.h"

//Qt
#include "vtkGenericOpenGLRenderWindow.h"
#include "QVTKOpenGLNativeWidget.h"

//vtk
#include "vtkRenderer.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"

#include "vtkChartBox.h"
#include "vtkTable.h"
#include "vtkPlot.h"
#include "vtkTextProperty.h"
#include "vtkAxis.h"

#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"

#include "vtkComputeQuartiles.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkLookupTable.h"

iACompBoxPlot::iACompBoxPlot(MainWindow* parent, iACsvDataStorage* dataStorage) : 
	QDockWidget(parent),
	m_dataStorage(dataStorage)
{
	//TODO draw boxplot
	setupUi(this);

	this->setFeatures(DockWidgetVerticalTitleBar);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	m_qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(m_qvtkWidget);

	m_view = vtkSmartPointer<vtkContextView>::New();
	m_view->SetRenderWindow(m_qvtkWidget->GetRenderWindow());
	m_view->SetInteractor(m_qvtkWidget->GetInteractor());

}

void iACompBoxPlot::showEvent(QShowEvent* event)
{
	vtkSmartPointer<vtkChartBox> chart = vtkSmartPointer<vtkChartBox>::New();
	m_view->GetScene()->AddItem(chart);

	// Creates a vtkPlotBox input table
	int numParam = 5;
	vtkSmartPointer<vtkTable> inputBoxPlotTable = vtkSmartPointer<vtkTable>::New();

	for (int i = 0; i < numParam; i++)
	{
		char num[10];
		sprintf(num, "Run %d", i + 1);
		vtkSmartPointer<vtkIntArray> arrIndex = vtkSmartPointer<vtkIntArray>::New();
		arrIndex->SetName(num);
		inputBoxPlotTable->AddColumn(arrIndex);
	}

	inputBoxPlotTable->SetNumberOfRows(20);
	double values[20][5] =
	{
	  {850,     960,     880,     890,     890},
	  {740,     940,     880,     810,     840},
	  {900,     960,     880,     810,     780},
	  {1070,    940,     860,     820,     810},
	  {930,     880,     720,     800,     760},
	  {850,     800,     720,     770,     810},
	  {950,     850,     620,     760,     790},
	  {980,     880,     860,     740,     810},
	  {980,     900,     970,     750,     820},
	  {880,     840,     950,     760,     850},
	  {1000,    830,     880,     910,     870},
	  {980,     790,     910,     920,     870},
	  {930,     810,     850,     890,     810},
	  {650,     880,     870,     860,     740},
	  {760,     880,     840,     880,     810},
	  {810,     830,     840,     720,     940},
	  {1000,    800,     850,     840,     950},
	  {1000,    790,     840,     850,     800},
	  {960,     760,     840,     850,     810},
	  {960,     800,     840,     780,     870}
	};
	for (int j = 0; j < 20; ++j)
	{
		for (int i = 0; i < 5; ++i)
		{
			inputBoxPlotTable->SetValue(j, i, values[j][i]);
		}
	}
	vtkSmartPointer<vtkComputeQuartiles> quartiles =
		vtkSmartPointer<vtkComputeQuartiles>::New();
	quartiles->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, inputBoxPlotTable);
	quartiles->Update();

	vtkTable *outTable = quartiles->GetOutput();
	vtkSmartPointer<vtkLookupTable> lookup =
		vtkSmartPointer<vtkLookupTable>::New();
	lookup->SetNumberOfColors(5);
	lookup->SetRange(0, 4);
	lookup->Build();

	chart->GetPlot(0)->SetInputData(outTable);
	chart->SetShowLegend(true);
	chart->SetColumnVisibilityAll(true);
	chart->SetTitle("Michelson-Morley experiment");
	chart->GetTitleProperties()->SetFontSize(16);
	chart->GetYAxis()->SetTitle("Speed of Light (km/s - 299000)");

	// Set the labels
	vtkSmartPointer<vtkStringArray> labels =
		vtkSmartPointer<vtkStringArray>::New();
	labels->SetNumberOfValues(5);
	labels->SetValue(0, "Run 1");
	labels->SetValue(1, "Run 2");
	labels->SetValue(2, "Run 3");
	labels->SetValue(3, "Run 4");
	labels->SetValue(4, "Run 5");
	chart->GetPlot(0)->SetLabels(labels);

	renderWidget();
}

void iACompBoxPlot::renderWidget()
{
	m_qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
}