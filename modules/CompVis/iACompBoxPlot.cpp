#include "iACompBoxPlot.h"

//CompVis
#include "iACompVisOptions.h"

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
#include "vtkChart.h"
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
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include "vtkPlotBox.h"
#include "vtkChartXY.h"
#include "vtkBrush.h"

#include "vtkTextActor.h"

#include <string>

iACompBoxPlot::iACompBoxPlot(MainWindow* parent, iACsvDataStorage* dataStorage) : 
	QDockWidget(parent),
	m_dataStorage(dataStorage),
	maxValsAttr(new std::vector<double>()),
	minValsAttr(new std::vector<double>()),
	inputBoxPlotTable(vtkSmartPointer<vtkTable>::New())
{
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
	// data preparation
	QList<csvFileData>* data = m_dataStorage->getData();
	int numParam = data->at(0).header->size();
	QStringList* attrNames = m_dataStorage->getData()->at(0).header;

	//create chart (a plot is stored inside a chart)
	vtkSmartPointer<vtkChartBox> chart = vtkSmartPointer<vtkChartBox>::New();
	chart->Update();
	m_view->GetScene()->AddItem(chart);

	// Set the labels
	vtkSmartPointer<vtkStringArray> labels = vtkSmartPointer<vtkStringArray>::New();
	
	//set amount of attributes
	for (int i = 0; i < numParam; i++)
	{
		labels->InsertNextValue(attrNames->at(i).toStdString());

		vtkSmartPointer<vtkIntArray> arrIndex = vtkSmartPointer<vtkIntArray>::New();
		arrIndex->SetName(attrNames->at(i).toStdString().c_str());
		inputBoxPlotTable->AddColumn(arrIndex);
	}

	//calculate amount of objects(fibers)/rows
	int numberOfRows = 0;
	for(int i = 0; i < data->size(); i++)
	{
		numberOfRows += csvDataType::getRows(data->at(i).values);
	}
	inputBoxPlotTable->SetNumberOfRows(numberOfRows);
	
	//fill table with data
	for (int i = 0; i < data->size(); i++)
	{//for all datasets
		for(int dataInd = 0; dataInd < data->at(i).values->size(); dataInd++)
		{ //for all values
			for (int attrInd = 0; attrInd < numParam; attrInd++) 
			{
				int row = i + dataInd;
				int col = attrInd;
				double val = data->at(i).values->at(dataInd).at(attrInd);
				inputBoxPlotTable->SetValue(row, col, val);
			}
		}
	}

	//calculate quartiles
	vtkSmartPointer<vtkComputeQuartiles> quartiles = vtkSmartPointer<vtkComputeQuartiles>::New();
	quartiles->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, inputBoxPlotTable);
	quartiles->Update();

	vtkSmartPointer<vtkTable> outTable = quartiles->GetOutput();
	vtkSmartPointer<vtkTable> normalizedTable = vtkSmartPointer<vtkTable>::New();
	normalizedTable->DeepCopy(outTable);

	//normalize quartiles in the interval [0,1]
	for (vtkIdType c = 0; c < outTable->GetNumberOfColumns(); c++)
	{
		for (vtkIdType r = 0; r < outTable->GetNumberOfRows(); r++)
		{
			vtkVariant v = outTable->GetValue(r, c);
			double newV = iACompVisOptions::histogramNormalization(v.ToDouble(), 0.0, 1.0, outTable->GetValue(0, c).ToDouble(), outTable->GetValue(outTable->GetNumberOfRows()-1, c).ToDouble());
			normalizedTable->SetValue(r, c, vtkVariant(newV));
		}
	}

	//create lookup table
	vtkSmartPointer<vtkLookupTable> lookup = vtkSmartPointer<vtkLookupTable>::New();
	lookup->SetNumberOfColors(2);
	lookup->SetRange(0, 1);
	lookup->Build();
	double col[3];
	col[0] = iACompVisOptions::BACKGROUNDCOLOR_GREY[0];
	col[1] = iACompVisOptions::BACKGROUNDCOLOR_GREY[1];
	col[2] = iACompVisOptions::BACKGROUNDCOLOR_GREY[2];
	lookup->SetTableValue(0, col[0], col[1], col[2]);
	lookup->SetTableValue(1, col[0], col[1], col[2]);

	//create box plot
	vtkSmartPointer<vtkPlotBox> box = vtkSmartPointer<vtkPlotBox>::New();
	box->SetInputData(normalizedTable);
	box->SetLookupTable(lookup);
	
	chart->SetPlot(box);
	chart->SetColumnVisibilityAll(true);
	chart->Update();

	//render all to get position of columns
	renderWidget();

	//set size of chart
	chart->SetPoint1(m_qvtkWidget->width()*0.0, (m_qvtkWidget->height()*0.4));
	chart->SetPoint2(m_qvtkWidget->width()*0.75, (m_qvtkWidget->height())*0.85);
	chart->Update();

	//create title
	chart->SetTitle("Interval Similarity");
	chart->GetTitleProperties()->BoldOn();
	chart->GetTitleProperties()->ItalicOff();
	chart->GetTitleProperties()->ShadowOff();
	chart->GetTitleProperties()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	chart->GetTitleProperties()->SetFontFamilyToArial();
	chart->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	chart->GetTitleProperties()->Modified();
	
	//create y axis
	vtkAxis *axisLeft = chart->GetYAxis();
	axisLeft->SetBehavior(1);
	axisLeft->SetTitle("Converted Interval Size");
	axisLeft->GetTitleProperties()->ItalicOff();
	axisLeft->GetTitleProperties()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	axisLeft->GetTitleProperties()->SetFontFamilyToArial();
	axisLeft->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	axisLeft->GetTitleProperties()->SetLineOffset(-20);
	axisLeft->GetTitleProperties()->SetOrientation(90);
	axisLeft->GetTitleProperties()->SetVerticalJustification(VTK_TEXT_CENTERED);
	axisLeft->GetTitleProperties()->SetJustification(VTK_TEXT_CENTERED);
	axisLeft->SetMaximum(1.0);
	axisLeft->SetMinimum(0.0);
	axisLeft->AutoScale();
	axisLeft->SetNumberOfTicks(5);

	//for each column draw a legend
	for (int i = 0; i < numParam; i++)
	{
		float xPos = chart->GetXPosition(i) + (box->GetBoxWidth()*0.5);
		double pos = ((double)xPos) / ((double)m_qvtkWidget->width());

		vtkSmartPointer<vtkTextActor> legend = vtkSmartPointer<vtkTextActor>::New();
		legend->SetTextScaleModeToNone();
		legend->SetInput(labels->GetValue(i));
		legend->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
		legend->GetPositionCoordinate()->SetValue(pos, 0.3);

		vtkSmartPointer<vtkTextProperty> legendProperty = legend->GetTextProperty();
		legendProperty->BoldOff();
		legendProperty->ItalicOff();
		legendProperty->ShadowOff();
		legendProperty->SetFontFamilyToArial();
		legendProperty->SetColor(0, 0, 0);
		legendProperty->SetOrientation(25);
		legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
		legendProperty->SetVerticalJustificationToCentered(); 
		legendProperty->SetJustification(VTK_TEXT_RIGHT);
		legendProperty->Modified();

		m_view->GetRenderer()->AddActor(legend);
	}
}

void iACompBoxPlot::renderWidget()
{
	m_qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
}