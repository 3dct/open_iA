#include "iACompBarChart.h"

//compVis
#include "iACoefficientOfVariation.h"
#include "iACompVisOptions.h"

//Qt
#include "vtkGenericOpenGLRenderWindow.h"
#include "QVTKOpenGLNativeWidget.h"

//vtk
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"

#include "vtkRenderer.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkChartXY.h"
#include "vtkTable.h"
#include "vtkPlotBar.h"
#include "vtkAxis.h"
#include "vtkTextProperty.h"

#include "mainwindow.h"
#include <vector>
#include <algorithm>

iACompBarChart::iACompBarChart(MainWindow* parent, iACoefficientOfVariation* coeffVar, iACsvDataStorage* dataStorage) :
	QDockWidget(parent), 
	m_coeffVar(coeffVar),
	m_dataStorage(dataStorage),
	m_renderer(vtkSmartPointer<vtkRenderer>::New()),
	orderedPositions(new std::vector<double>())
{
	//todo add interaction
	setupUi(this);
	this->setFeatures(DockWidgetVerticalTitleBar);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	m_qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(m_qvtkWidget);

	m_view = vtkSmartPointer<vtkContextView>::New();
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	m_view->SetRenderWindow(m_qvtkWidget->GetRenderWindow());
	m_view->SetInteractor(m_qvtkWidget->GetInteractor());
#else
	m_view->SetRenderWindow(m_qvtkWidget->renderWindow());
	m_view->SetInteractor(m_qvtkWidget->interactor());
#endif

	//data preparation
	attrNames = m_dataStorage->getData()->at(0).header;
	std::vector<double>* coefficientsOriginal = m_coeffVar->getCoefficientOfVariation();
	removeLabelAttribute(coefficientsOriginal, attrNames);

	//change interval from [0,1] to [0,100]
	coefficients = changeInterval(coefficientsOriginal, 100.0, 0.0, 1.0, 0.0);
	orderedPositions = sortWithMemory(coefficients);
}

void iACompBarChart::showEvent(QShowEvent* event)
{
	// Set up a 2D scene, add an XY chart to it
	vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
	m_view->GetScene()->AddItem(chart);

	// Create table for data
	char* coeffName = "Coefficients";
	char* indName = "Indices";
	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
	table->AddColumn(getIndexArray(coefficients, indName));
	table->AddColumn(vectorToVtkDataArray(coefficients, coeffName));
	
	//bars
	vtkPlotBar *bar = 0;
	bar = vtkPlotBar::SafeDownCast(chart->AddPlot(vtkChart::BAR));
	double* barColor = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY);
	bar->SetColor(barColor[0], barColor[1], barColor[2]);
	bar->SetInputData(table, indName, coeffName);
	bar->SetTooltipLabelFormat("%l: %y");
	
	chart->SetShowLegend(true);

	//bottom axis
	vtkAxis *axisBottom = chart->GetAxis(vtkAxis::BOTTOM);
	axisBottom->SetBehavior(1);
	axisBottom->SetShift(20);
	axisBottom->Update();
	axisBottom->SetMaximum(coefficients->size()+1); //add 1 since the all bars are drawn from 1 to end (and not from 0!)
	axisBottom->SetTitle("Attributes");
	axisBottom->GetTitleProperties()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	axisBottom->GetTitleProperties()->SetFontFamilyToArial();
	axisBottom->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);

	//labels on bottom axis
	vtkSmartPointer<vtkDoubleArray> labelInd = vtkSmartPointer<vtkDoubleArray>::New();
	vtkSmartPointer<vtkStringArray> labelStrings = vtkSmartPointer<vtkStringArray>::New();
	
	for (int i = 0; i < coefficients->size(); i++)
	{
		labelInd->InsertNextValue(i+1); //start with 1 so that the first bar is not drawn inside y-axis
		labelStrings->InsertNextValue(attrNames->at(orderedPositions->at(i)).toStdString());
	}

	axisBottom->SetCustomTickPositions(labelInd, labelStrings);
	axisBottom->GetLabelProperties()->SetLineOffset(10);
	axisBottom->GetLabelProperties()->SetOrientation(20);
	axisBottom->GetLabelProperties()->BoldOff();
	axisBottom->GetLabelProperties()->ItalicOff();
	axisBottom->GetLabelProperties()->SetFontFamilyToArial();
	axisBottom->GetLabelProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TEXT); 
	axisBottom->GetLabelProperties()->SetVerticalJustification(VTK_TEXT_CENTERED);
	axisBottom->GetLabelProperties()->SetJustification(VTK_TEXT_RIGHT);

	//left axis
	vtkAxis *axisLeft = chart->GetAxis(vtkAxis::LEFT);
	axisLeft->SetBehavior(1);
	axisLeft->SetTitle("Similarity in %");
	axisLeft->GetTitleProperties()->ItalicOff();
	axisLeft->GetTitleProperties()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	axisLeft->GetTitleProperties()->SetFontFamilyToArial();
	axisLeft->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	axisLeft->GetTitleProperties()->SetLineOffset(-20);
	axisLeft->GetTitleProperties()->SetOrientation(90);
	axisLeft->GetTitleProperties()->SetVerticalJustification(VTK_TEXT_CENTERED);
	axisLeft->GetTitleProperties()->SetJustification(VTK_TEXT_CENTERED);
	axisLeft->SetMaximum(100.0);
	axisLeft->SetMinimum(0.0);
	axisLeft->AutoScale();
	axisLeft->SetNumberOfTicks(5);

	//chart title
	chart->SetTitle("Coefficient of Variation");
	chart->GetTitleProperties()->BoldOn();
	chart->GetTitleProperties()->ItalicOff();
	chart->GetTitleProperties()->ShadowOff();
	chart->GetTitleProperties()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	chart->GetTitleProperties()->SetFontFamilyToArial();
	chart->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	chart->GetTitleProperties()->Modified();

	//chart legend
	chart->SetShowLegend(false);

	this->renderWidget();	
}

void iACompBarChart::updateBarChart(std::vector<double>* coefficients)
{
	//TODO implement update!
}

std::vector<double>* iACompBarChart::changeInterval(std::vector<double>* input, double newMax, double newMin, double oldMax, double oldMin)
{
	std::vector<double>* result = new std::vector<double>(input->size(), 0);

	for (int i = 0; i < input->size(); i++)
	{
		double val = iACompVisOptions::histogramNormalization(input->at(i), newMin, newMax, oldMin, oldMax);
		result->at(i) = val;
	}

	return result;
}
	
vtkSmartPointer<vtkIntArray> iACompBarChart::getIndexArray(std::vector<double>* input, const char* name)
{
	vtkSmartPointer<vtkIntArray> result = vtkSmartPointer<vtkIntArray>::New();
	result->SetName(name);
	for (int i = 1; i <= input->size(); i++)
	{//start from one to draw bar not inside y-axis
		result->InsertNextValue(i);
	}

	return result;
}

vtkSmartPointer<vtkDoubleArray> iACompBarChart::vectorToVtkDataArray(std::vector<double>* input, const char* name)
{
	vtkSmartPointer<vtkDoubleArray> result = vtkSmartPointer<vtkDoubleArray>::New();
	result->SetName(name);
	for (int i = 0; i < input->size(); i++)
	{
		result->InsertNextTuple(&input->at(i));
	}

	return result;
}

std::vector<double>* iACompBarChart::sortWithMemory(std::vector<double>* input)
{
	std::vector<double>* newPositions = new std::vector<double>(input->size(), 0);
	int n(0);
	std::generate(std::begin(*newPositions), std::end(*newPositions), [&] { return n++; });

	auto comparator = [input](int i1, int i2) { return input->at(i1) > input->at(i2);};
	std::sort(std::begin(*newPositions), std::end(*newPositions), comparator);
	std::sort(std::begin(*input), std::end(*input), std::greater<double>());

	return newPositions;
}

void iACompBarChart::removeLabelAttribute(std::vector<double>* input, QStringList* names)
{//remove the first attribute, since this contains only the label number
	names->removeFirst();
	input->erase(input->begin());
}

void iACompBarChart::renderWidget()
{
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	m_qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
#else
	m_qvtkWidget->renderWindow()->GetInteractor()->Render();
#endif
}

std::vector<double>* iACompBarChart::getOrderedPositions()
{
	return orderedPositions;
}