// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompBarChart.h"

//compVis
#include "iACoefficientOfVariation.h"
#include "iACompVisOptions.h"

// core
#include "iAMainWindow.h"
#include "iAQVTKWidget.h"

//vtk
#include <vtkAbstractContextItem.h>
#include <vtkActor.h>
#include <vtkAxis.h>
#include <vtkBoundingBox.h>
#include <vtkBrush.h>
#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkContextArea.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkCoordinate.h>
#include <vtkCubeSource.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkGlyph3DMapper.h>
#include <vtkIntArray.h>
#include <vtkLookupTable.h>
#include <vtkObjectFactory.h> //for macro!
#include <vtkPen.h>
#include <vtkPlaneSource.h>
#include <vtkPlotBar.h>
#include <vtkPointData.h>
#include <vtkPointLocator.h>
#include <vtkPointPicker.h>
#include <vtkPolyDataItem.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolygon.h>
#include <vtkProperty.h>
#include <vtkPropItem.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>
#include <vtkTooltipItem.h>

#include <QVBoxLayout>

#include <vector>
#include <algorithm>

vtkStandardNewMacro(iACompBarChart::BarChartInteractorStyle);

iACompBarChart::iACompBarChart(iAMainWindow* parent, iACoefficientOfVariation* coeffVar, iACsvDataStorage* dataStorage) :
	QDockWidget(parent),
	m_dataStorage(dataStorage),
	m_coeffVar(coeffVar),
	m_qvtkWidget(new iAQVTKWidget(this)),
	m_area(vtkSmartPointer<vtkContextArea>::New()),
	orderedPositions(new std::vector<double>()),
	selected_orderedPositions(nullptr),
	m_originalBarChart(vtkSmartPointer<vtkPropItem>::New()),
	m_originalBarChartRepositioned(nullptr),
	m_selectedBarChart(vtkSmartPointer<vtkPropItem>::New()),
	m_lastState(iACompVisOptions::lastState::Undefined)
{
	setFeatures(DockWidgetVerticalTitleBar);
	setWindowTitle("Bar Chart");
	setWidget(new QWidget());
	widget()->setLayout(new QVBoxLayout());
	widget()->layout()->addWidget(m_qvtkWidget);

	//initialize interaction
	style = vtkSmartPointer<BarChartInteractorStyle>::New();
	style->setOuterClass(this);
	style->initializeBarChartInteractorStyle();

	m_view = vtkSmartPointer<vtkContextView>::New();

	m_qvtkWidget->interactor()->SetInteractorStyle(style);

	m_view->SetRenderWindow(m_qvtkWidget->renderWindow());
	m_view->SetInteractor(m_qvtkWidget->interactor());
	m_view->GetInteractor()->SetInteractorStyle(style);

	//data preparation
	attrNames = m_dataStorage->getAttributeNamesWithoutLabel();
	std::vector<double>* coefficientsOriginal = m_coeffVar->getCoefficientOfVariation();
	removeLabelAttribute(coefficientsOriginal);

	//change interval from [0,1] to [0,100]
	coefficients = changeInterval(coefficientsOriginal, 100.0, 0.0, 1.0, 0.0);
	coefficientsUnordered = new std::vector<double>(coefficients->size(), 0);
	iACompVisOptions::copyVector(coefficients, coefficientsUnordered);

	orderedPositions = sortWithMemory(coefficients);
}

void iACompBarChart::showEvent(QShowEvent* event)
{
	Q_UNUSED(event);
	if (m_lastState == iACompVisOptions::lastState::Undefined)
	{
		initializeBarChart();
	}
	else if (m_lastState == iACompVisOptions::lastState::Defined)
	{
		renderWidget();
	}
}

void iACompBarChart::initializeBarChart()
{
	int numberOfBars = static_cast<int>(coefficients->size());

	vtkBoundingBox bounds = vtkBoundingBox();
	bounds.SetBounds(0, numberOfBars + 1, 0, 100, 0, 0);
	m_area->SetDrawAreaBounds(vtkRectd(bounds.GetBound(0), bounds.GetBound(2),
		bounds.GetLength(0), bounds.GetLength(1)));

	auto barPositionsOriginal = vtkSmartPointer<vtkPoints>::New();

	auto scalesOriginal = vtkSmartPointer<vtkFloatArray>::New();
	std::string scaleArrayName = "ScalesOriginal";
	scalesOriginal->SetName(scaleArrayName.c_str());
	scalesOriginal->SetNumberOfComponents(3);

	auto colorsOriginal = vtkSmartPointer<vtkUnsignedCharArray>::New();
	std::string colorArrayName = "ColorsOriginal";
	colorsOriginal->SetName(colorArrayName.c_str());
	colorsOriginal->SetNumberOfComponents(3);

	for (int i = 0; i < numberOfBars; i++)
	{
		double xi = i + 1;
		double yi = 0;

		//set position
		barPositionsOriginal->InsertNextPoint(xi, yi, 0.0);

		//set scale
		scalesOriginal->InsertNextTuple3(m_barWidth, 2 * coefficientsUnordered->at(orderedPositions->at(i)), 1);

		//set color
		colorsOriginal->InsertNextTypedTuple(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY);
	}

	auto polyData = vtkSmartPointer<vtkPolyData>::New();
	polyData->SetPoints(barPositionsOriginal);
	polyData->GetPointData()->AddArray(colorsOriginal);
	polyData->GetPointData()->AddArray(scalesOriginal);

	//initialize for interaction
	style->buildPointLocatorOriginal(polyData);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);
	m_originalBarChart = addBars(polyData, colorArrayName, scaleArrayName, 1, col);

	m_view->GetScene()->AddItem(m_area);
	m_view->GetRenderWindow()->SetMultiSamples(0);
	m_view->Update();

	initializeAxes(orderedPositions);

	m_lastState = iACompVisOptions::lastState::Defined;
}

void iACompBarChart::initializeAxes(std::vector<double>* orderedPos)
{
	//axes not needed
	m_area->GetAxis(vtkAxis::RIGHT)->SetAxisVisible(false);
	m_area->GetAxis(vtkAxis::RIGHT)->SetLabelsVisible(false);
	m_area->GetAxis(vtkAxis::RIGHT)->SetTicksVisible(false);

	//bottom axis
	vtkAxis *axisBottom = m_area->GetAxis(vtkAxis::BOTTOM);
	axisBottom->SetBehavior(1);
	axisBottom->SetShift(20);
	axisBottom->Update();
	axisBottom->SetMaximum(coefficients->size() + 1); //add 1 since the all bars are drawn from 1 to end (and not from 0!)
	axisBottom->SetTitle("Attributes");
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);
	axisBottom->GetTitleProperties()->SetColor(col);
	axisBottom->GetTitleProperties()->SetFontFamilyToArial();
	axisBottom->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);

	//labels on bottom axis
	auto labelInd = vtkSmartPointer<vtkDoubleArray>::New();
	auto labelStrings = vtkSmartPointer<vtkStringArray>::New();

	for (int i = 0; i < ((int)coefficients->size()); i++)
	{
		labelInd->InsertNextValue(i + 1); //start with 1 so that the first bar is not drawn inside y-axis
		labelStrings->InsertNextValue(attrNames->at(orderedPos->at(i)).toStdString());
	}

	axisBottom->SetCustomTickPositions(labelInd, labelStrings);
	axisBottom->GetLabelProperties()->SetLineOffset(10);


	if (coefficients->size() < 20)
	{
		axisBottom->GetLabelProperties()->SetOrientation(20); //use with material data
	}
	else {
		axisBottom->GetLabelProperties()->SetOrientation(30);//use with unemployment data
	}

	axisBottom->GetLabelProperties()->BoldOff();
	axisBottom->GetLabelProperties()->ItalicOff();
	axisBottom->GetLabelProperties()->SetFontFamilyToArial();
	axisBottom->GetLabelProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	axisBottom->GetLabelProperties()->SetVerticalJustification(VTK_TEXT_CENTERED);
	axisBottom->GetLabelProperties()->SetJustification(VTK_TEXT_RIGHT);
	axisBottom->SetGridVisible(true);
	axisBottom->Update();

	//left axis
	vtkAxis *axisLeft = m_area->GetAxis(vtkAxis::LEFT);
	axisLeft->SetBehavior(1);
	axisLeft->SetTitle("Similarity in %");
	axisLeft->GetTitleProperties()->ItalicOff();
	double col1[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col1);
	axisLeft->GetTitleProperties()->SetColor(col1);
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
	axisLeft->SetGridVisible(true);
	axisLeft->Update();

	//chart title
	m_area->GetAxis(vtkAxis::TOP)->SetAxisVisible(false);
	m_area->GetAxis(vtkAxis::TOP)->SetTicksVisible(false);
	m_area->GetAxis(vtkAxis::TOP)->SetLabelsVisible(false);
	m_area->GetAxis(vtkAxis::TOP)->SetTitleVisible(true);
	vtkAxis *axisTop = m_area->GetAxis(vtkAxis::TOP);
	axisTop->SetTitle("Coefficient of Variation");
	axisTop->GetTitleProperties()->BoldOn();
	axisTop->GetTitleProperties()->ItalicOff();
	axisTop->GetTitleProperties()->ShadowOff();
	double col2[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col2);
	axisTop->GetTitleProperties()->SetColor(col2);
	axisTop->GetTitleProperties()->SetFontFamilyToArial();
	axisTop->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	axisTop->GetTitleProperties()->Modified();
	axisTop->Update();

	m_area->SetShowGrid(true);
	m_area->Update();
	m_view->Update();
}

vtkSmartPointer<vtkPropItem> iACompBarChart::addBars(vtkSmartPointer<vtkPolyData> polyData, std::string colorArrayName, std::string scaleArrayName, double opacity, double col[3])
{
	auto cubeSource = vtkSmartPointer<vtkCubeSource>::New();

	auto glyph3Dmapper = vtkSmartPointer<vtkGlyph3DMapper>::New();
	glyph3Dmapper->SetSourceConnection(cubeSource->GetOutputPort());
	glyph3Dmapper->SetInputData(polyData);
	glyph3Dmapper->SetScalarModeToUsePointFieldData();
	glyph3Dmapper->SetScaleArray(scaleArrayName.c_str());
	glyph3Dmapper->SetScaleModeToScaleByVectorComponents();
	glyph3Dmapper->SelectColorArray(colorArrayName.c_str());
	glyph3Dmapper->Update();

	auto glyphActor = vtkSmartPointer<vtkActor>::New();
	glyphActor->SetMapper(glyph3Dmapper);
	glyphActor->GetProperty()->SetEdgeVisibility(true);

	glyphActor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
	glyphActor->GetProperty()->SetLineWidth(2);
	glyphActor->GetProperty()->SetOpacity(opacity);
	glyphActor->PickableOn();

	auto propItem = vtkSmartPointer<vtkPropItem>::New();
	propItem->SetPropObject(glyphActor);

	m_area->GetDrawAreaItem()->AddItem(propItem);

	return propItem;
}

std::vector<double>* iACompBarChart::changeInterval(std::vector<double>* input, double newMax, double newMin, double oldMax, double oldMin)
{

	std::vector<double>* result = new std::vector<double>(input->size(), 0);

	/*LOG(lvlDebug,"oldMin = " + QString::number(oldMin));
	LOG(lvlDebug,"oldMax = " + QString::number(oldMax));*/

	for (int i = 0; i < ((int)input->size()); i++)
	{
		//LOG(lvlDebug,"input->at(i) = " + QString::number(input->at(i)));
		double val = iACompVisOptions::histogramNormalization(input->at(i), newMin, newMax, oldMin, oldMax);
		result->at(i) = val;
	}

	return result;
}

vtkSmartPointer<vtkIntArray> iACompBarChart::getIndexArray(std::vector<double>* input, const char* name)
{
	auto result = vtkSmartPointer<vtkIntArray>::New();
	result->SetName(name);
	for (int i = 1; i <= ((int)input->size()); i++)
	{//start from one to draw bar not inside y-axis
		result->InsertNextValue(i);
	}

	return result;
}

vtkSmartPointer<vtkDoubleArray> iACompBarChart::vectorToVtkDataArray(std::vector<double>* input, const char* name)
{
	auto result = vtkSmartPointer<vtkDoubleArray>::New();
	result->SetName(name);
	for (int i = 0; i < ((int)input->size()); i++)
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

void iACompBarChart::removeLabelAttribute(std::vector<double>* input)
{//remove the first attribute, since this contains only the label number
	input->erase(input->begin());
}

void iACompBarChart::renderWidget()
{
	m_qvtkWidget->renderWindow()->GetInteractor()->Render();
}

/******************************************  Getter Methods  **********************************************/

std::vector<double>* iACompBarChart::getOrderedPositions()
{
	return orderedPositions;
}

std::vector<double>* iACompBarChart::getSelectedOrderedPositions()
{
	return selected_orderedPositions;
}

/******************************************  Update Methods  **********************************************/

void iACompBarChart::updateBarChart(std::vector<double>* coefficientsOriginal, std::map<int, std::vector<double>>* pickStatistic)
{
	//reset bar chart
	m_area->GetDrawAreaItem()->RemoveItem(m_selectedBarChart);
	m_area->GetDrawAreaItem()->RemoveItem(m_originalBarChartRepositioned);
	style->resetPointLocatorSelected();
	style->hideTooltip();

	std::vector<double>* coefficientsSelected = new std::vector<double>();

	if(coefficientsOriginal->size() == 0)
	{ // no data -> draw empty bar chart

		for(int i = 0; i < attrNames->size(); i++)
		{
			coefficientsSelected->push_back(0);
		}

	}else
	{
		//delete label column
		coefficientsOriginal->erase(coefficientsOriginal->begin());

		//change interval from [0,1] to [0,100]
		coefficientsSelected = changeInterval(coefficientsOriginal, 100.0, 0.0, 1.0, 0.0);
	}

	selected_orderedPositions = sortWithMemory(coefficientsSelected);

	updateOriginalBarChart();
	updateLabels();

	//double maxVal = coefficientsSelected->at(0); //first value is the biggest
	double maxNumberObjects = m_dataStorage->getTotalNumberOfObjects();
	double selectedNumberObjects = 0;
	for (auto iter = pickStatistic->begin(); iter != pickStatistic->end(); ++iter)
	{
		selectedNumberObjects += ((double) iter->second.at(1));
	}

	//calculate width of selected
	double width = (m_barWidth / maxNumberObjects) * selectedNumberObjects;
	int numberOfBars = static_cast<int>(coefficientsSelected->size());

	auto barPositionsSelected = vtkSmartPointer<vtkPoints>::New();

	auto scales = vtkSmartPointer<vtkFloatArray>::New();
	std::string scaleArrayName = "ScalesArraySelected";
	scales->SetName(scaleArrayName.c_str());
	scales->SetNumberOfComponents(3);

	auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	std::string colorArrayName = "ColorsArraySelected";
	colors->SetName(colorArrayName.c_str());
	colors->SetNumberOfComponents(3);

	for (int i = 0; i < numberOfBars; i++)
	{
		double xi = i + 1;
		double yi = 0;

		//set position
		double xiSelected = xi - (m_barWidth * 0.5) + (width * 0.5);
		barPositionsSelected->InsertNextPoint(xiSelected, yi, 0.0);

		//set scale
		scales->InsertNextTuple3(width, 2*coefficientsSelected->at(i), 1);

		//set color
		colors->InsertNextTypedTuple(iACompVisOptions::HIGHLIGHTCOLOR_GREEN);
	}

	auto polyData = vtkSmartPointer<vtkPolyData>::New();
	polyData->SetPoints(barPositionsSelected);
	polyData->GetPointData()->AddArray(colors);
	polyData->GetPointData()->AddArray(scales);

	//initialize for interaction
	style->buildPointLocatorSelected(polyData);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);

	m_selectedBarChart = addBars(polyData, colorArrayName, scaleArrayName, 0.5, col);

	renderWidget();
}

void iACompBarChart::updateOriginalBarChart()
{
	m_area->GetDrawAreaItem()->RemoveItem(m_originalBarChart);

	int numberOfBars = static_cast<int>(coefficients->size());

	auto barPositionsOriginal = vtkSmartPointer<vtkPoints>::New();

	auto scalesOriginal = vtkSmartPointer<vtkFloatArray>::New();
	std::string scaleArrayName = "ScalesOriginalRepositioned";
	scalesOriginal->SetName(scaleArrayName.c_str());
	scalesOriginal->SetNumberOfComponents(3);

	auto colorsOriginal = vtkSmartPointer<vtkUnsignedCharArray>::New();
	std::string colorArrayName = "ColorsOriginalRepositioned";
	colorsOriginal->SetName(colorArrayName.c_str());
	colorsOriginal->SetNumberOfComponents(3);

	for (int i = 0; i < numberOfBars; i++)
	{
		double xi = i + 1;
		double yi = 0;

		//set position
		barPositionsOriginal->InsertNextPoint(xi, yi, 0.0);

		//set scale
		scalesOriginal->InsertNextTuple3(m_barWidth, 2 * coefficientsUnordered->at(selected_orderedPositions->at(i)), 1);

		//set color
		colorsOriginal->InsertNextTypedTuple(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY);
	}

	auto polyData = vtkSmartPointer<vtkPolyData>::New();
	polyData->SetPoints(barPositionsOriginal);
	polyData->GetPointData()->AddArray(colorsOriginal);
	polyData->GetPointData()->AddArray(scalesOriginal);

	//initialize for interaction
	style->buildPointLocatorOriginalRepositioned(polyData);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);

	m_originalBarChartRepositioned = addBars(polyData, colorArrayName, scaleArrayName, 1, col);

}

void iACompBarChart::updateLabels()
{
	vtkAxis *axisBottom = m_area->GetAxis(vtkAxis::BOTTOM);

	//labels on bottom axis
	auto labelInd = vtkSmartPointer<vtkDoubleArray>::New();
	auto labelStrings = vtkSmartPointer<vtkStringArray>::New();

	for (int i = 0; i < ((int)coefficients->size()); i++)
	{
		labelInd->InsertNextValue(i + 1); //start with 1 so that the first bar is not drawn inside y-axis
		labelStrings->InsertNextValue(attrNames->at(selected_orderedPositions->at(i)).toStdString());
	}

	axisBottom->SetCustomTickPositions(labelInd, labelStrings);
	axisBottom->Update();
}

void iACompBarChart::resetBarChart()
{
	style->hideTooltip();
	style->resetPointLocatorSelected();

	//reset the labels
	resetLabels();

	if(m_area->GetDrawAreaItem()->GetItemIndex(m_originalBarChart) == -1)
	{
		m_area->GetDrawAreaItem()->AddItem(m_originalBarChart);
		style->buildPointLocatorOriginal(vtkPolyData::SafeDownCast(vtkActor::SafeDownCast(m_originalBarChart->GetPropObject())->GetMapper()->GetInput()));
	}

	m_area->GetDrawAreaItem()->RemoveItem(m_selectedBarChart);
	m_area->GetDrawAreaItem()->RemoveItem(m_originalBarChartRepositioned);
	m_area->Update();
	m_view->Update();

	renderWidget();
}

void iACompBarChart::resetLabels()
{
	vtkAxis *axisBottom = m_area->GetAxis(vtkAxis::BOTTOM);

	//labels on bottom axis
	auto labelInd = vtkSmartPointer<vtkDoubleArray>::New();
	auto labelStrings = vtkSmartPointer<vtkStringArray>::New();

	for (int i = 0; i < ((int)coefficients->size()); i++)
	{
		labelInd->InsertNextValue(i + 1); //start with 1 so that the first bar is not drawn inside y-axis
		labelStrings->InsertNextValue(attrNames->at(orderedPositions->at(i)).toStdString());
	}

	axisBottom->SetCustomTickPositions(labelInd, labelStrings);
	axisBottom->Update();
}

/******************************************  INNER CLASS BarChartInteractorStyle  ****************************/

iACompBarChart::BarChartInteractorStyle::BarChartInteractorStyle() {}

void iACompBarChart::BarChartInteractorStyle::initializeBarChartInteractorStyle()
{
	m_toolTip = vtkSmartPointer<vtkTooltipItem>::New();
	m_picker = vtkSmartPointer<vtkPointPicker>::New();
	pointLocatorOriginal = vtkSmartPointer<vtkPointLocator>::New();
	pointLocatorSelected = vtkSmartPointer<vtkPointLocator>::New();

	m_barIndexPositionPairOriginal = new std::map<int, std::vector<double>>();
	m_barIndexPositionPairSelected = new std::map<int, std::vector<double>>();
	m_barIndexPositionPairOriginalRepositioned = new std::map<int, std::vector<double>>();
	m_selectedPointLocatorEmpty = true;

	m_toolTip->SetVisible(false);
	m_outerClass->m_area->AddItem(m_toolTip);

	//set text properties
	vtkSmartPointer<vtkTextProperty> toolTipProperty = m_toolTip->GetTextProperties();
	toolTipProperty->BoldOff();
	toolTipProperty->ItalicOff();
	toolTipProperty->ShadowOff();
	toolTipProperty->SetFontFamilyToArial();
	toolTipProperty->SetColor(0, 0, 0);
	toolTipProperty->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	toolTipProperty->Modified();

	unsigned char r = iACompVisOptions::HIGHLIGHTCOLOR_YELLOW[0];
	unsigned char g = iACompVisOptions::HIGHLIGHTCOLOR_YELLOW[1];
	unsigned char b = iACompVisOptions::HIGHLIGHTCOLOR_YELLOW[2];
	m_toolTip->GetBrush()->SetColor(r,g,b,255);
}

void iACompBarChart::BarChartInteractorStyle::setOuterClass(iACompBarChart* outerClass)
{
	m_outerClass = outerClass;
}

void iACompBarChart::BarChartInteractorStyle::buildPointLocatorOriginal(vtkSmartPointer<vtkPolyData> polyData)
{
	m_barIndexPositionPairOriginal->clear();

	pointLocatorOriginal = vtkSmartPointer<vtkPointLocator>::New();
	pointLocatorOriginal->SetDataSet(polyData);
	pointLocatorOriginal->AutomaticOn();
	pointLocatorOriginal->SetNumberOfPointsPerBucket(1);
	pointLocatorOriginal->BuildLocator();

	for(int i = 0; i < polyData->GetNumberOfPoints(); i++)
	{
		int index = i;
		std::vector<double> positions = std::vector<double>(4,0);
		double xMiddle = polyData->GetPoint(i)[0];

		double width = polyData->GetPointData()->GetArray("ScalesOriginal")->GetTuple3(i)[0];
		double height = polyData->GetPointData()->GetArray("ScalesOriginal")->GetTuple3(i)[1] / 2;

		double xMin = xMiddle - (width*0.5);
		double xMax = xMiddle + (width*0.5);
		double yMin = 0;
		double yMax = height;

		positions.at(0) = xMin;
		positions.at(1) = xMax;
		positions.at(2) = yMin;
		positions.at(3) = yMax;

		m_barIndexPositionPairOriginal->insert({ index, positions });
	}
}

void iACompBarChart::BarChartInteractorStyle::buildPointLocatorSelected(vtkSmartPointer<vtkPolyData> polyData)
{
	m_selectedPointLocatorEmpty = false;
	m_barIndexPositionPairSelected->clear();

	pointLocatorSelected = vtkSmartPointer<vtkPointLocator>::New();
	pointLocatorSelected->SetDataSet(polyData);
	pointLocatorSelected->AutomaticOn();
	pointLocatorSelected->SetNumberOfPointsPerBucket(1);
	pointLocatorSelected->BuildLocator();

	for (int i = 0; i < polyData->GetNumberOfPoints(); i++)
	{
		int index = i;
		std::vector<double> positions = std::vector<double>(4, 0);
		double xMiddle = polyData->GetPoint(i)[0];

		double width = polyData->GetPointData()->GetArray("ScalesArraySelected")->GetTuple3(i)[0];
		double height = polyData->GetPointData()->GetArray("ScalesArraySelected")->GetTuple3(i)[1] / 2;

		double xMin = xMiddle - (width*0.5);
		double xMax = xMiddle + (width*0.5);
		double yMin = 0;
		double yMax = height;

		positions.at(0) = xMin;
		positions.at(1) = xMax;
		positions.at(2) = yMin;
		positions.at(3) = yMax;

		m_barIndexPositionPairSelected->insert({ index, positions });
	}
}

void iACompBarChart::BarChartInteractorStyle::buildPointLocatorOriginalRepositioned(vtkSmartPointer<vtkPolyData> polyData)
{
	m_barIndexPositionPairOriginalRepositioned->clear();

	pointLocatorSelected = vtkSmartPointer<vtkPointLocator>::New();
	pointLocatorSelected->SetDataSet(polyData);
	pointLocatorSelected->AutomaticOn();
	pointLocatorSelected->SetNumberOfPointsPerBucket(1);
	pointLocatorSelected->BuildLocator();

	for (int i = 0; i < polyData->GetNumberOfPoints(); i++)
	{
		int index = i;
		std::vector<double> positions = std::vector<double>(4, 0);
		double xMiddle = polyData->GetPoint(i)[0];

		double width = polyData->GetPointData()->GetArray("ScalesOriginalRepositioned")->GetTuple3(i)[0];
		double height = polyData->GetPointData()->GetArray("ScalesOriginalRepositioned")->GetTuple3(i)[1] / 2;

		double xMin = xMiddle - (width*0.5);
		double xMax = xMiddle + (width*0.5);
		double yMin = 0;
		double yMax = height;

		positions.at(0) = xMin;
		positions.at(1) = xMax;
		positions.at(2) = yMin;
		positions.at(3) = yMax;

		m_barIndexPositionPairOriginalRepositioned->insert({ index, positions });
	}
}

void iACompBarChart::BarChartInteractorStyle::resetPointLocatorSelected()
{
	pointLocatorSelected->Initialize();
	m_selectedPointLocatorEmpty = true;
}

void iACompBarChart::BarChartInteractorStyle::OnLeftButtonDown()
{
	const int* pos = GetInteractor()->GetEventPosition();
	FindPokedRenderer(pos[0], pos[1]);

	m_picker->Pick(pos[0], pos[1], 0, CurrentRenderer);

	vtkIdList* result = vtkIdList::New();
	result->SetNumberOfIds(1);

	const vtkVector2f posVec = { (float)pos[0], (float)pos[1] };
	vtkVector2f posInSceneCoords = m_outerClass->m_area->GetDrawAreaItem()->MapFromScene(posVec);

	double posInScene[3] = { posInSceneCoords[0], 0.0, 0.0 };

	if(m_selectedPointLocatorEmpty)
	{
		pointLocatorOriginal->FindClosestNPoints(1, posInScene, result);
		auto iter = m_barIndexPositionPairOriginal->find(result->GetId(0));
		if (iter != m_barIndexPositionPairOriginal->end())
		{
			setTooltipTextOriginalBarChart(iter->second);
			showToolTip(iter->second, posInScene);
		}
	}
	else
	{
		pointLocatorSelected->FindClosestNPoints(1, posInScene, result);
		auto iterOri = m_barIndexPositionPairOriginalRepositioned->find(result->GetId(0));
		auto iter = m_barIndexPositionPairSelected->find(result->GetId(0));
		if ((iterOri != m_barIndexPositionPairOriginalRepositioned->end()) && (iter != m_barIndexPositionPairSelected->end()))
		{
			setTooltipTextSelectedBarChart(iter->second, iterOri->second);
			showToolTip(iterOri->second, posInScene);
		}
	}

	m_outerClass->renderWidget();
}

void iACompBarChart::BarChartInteractorStyle::showToolTip(std::vector<double> positions, double* posInScene)
{
	double xMin = positions.at(0);
	double xMax = positions.at(1);
	//double yMin = positions.at(2);
	double yMax = positions.at(3);

	double pickedX = posInScene[0];
	//double pickedY = posInScene[1];

	if(pickedX >= xMin && pickedX <= xMax)
	{
		double plotPosX = xMin + ((xMax - xMin)*0.5);
		double plotPosY = yMax;

		const vtkVector2f posVec = { (float)plotPosX, (float)plotPosY };
		vtkVector2f posVecInWorldCoords = m_outerClass->m_area->GetDrawAreaItem()->MapToScene(posVec);

		double pos[2] = { (double)posVecInWorldCoords[0], posVecInWorldCoords[1] };

		m_toolTip->SetPosition(pos[0], pos[1]);
		m_toolTip->SetVisible(true);
	}
	else
	{
		m_toolTip->SetVisible(false);
	}
}

void iACompBarChart::BarChartInteractorStyle::setTooltip(vtkTooltipItem* tooltip)
{
	if (tooltip == m_toolTip)
	{
		// nothing to change
		return;
	}

	if (m_toolTip)
	{
		// remove current tooltip from scene
		m_outerClass->m_area->RemoveItem(m_toolTip);
	}

	m_toolTip = tooltip;

	if (m_toolTip)
	{
		// add new tooltip to scene
		m_outerClass->m_area->AddItem(m_toolTip);
	}
}

vtkTooltipItem* iACompBarChart::BarChartInteractorStyle::GetTooltip()
{
	return m_toolTip;
}

void iACompBarChart::BarChartInteractorStyle::setTooltipTextOriginalBarChart(std::vector<double> values)
{
	int percent = values.at(3);
	vtkStdString tooltipLabel = "" + std::to_string(percent) + " %";

	m_toolTip->SetText(tooltipLabel);
}

void iACompBarChart::BarChartInteractorStyle::setTooltipTextSelectedBarChart(std::vector<double> valuesSelected, std::vector<double> valuesOriginal)
{
	int percentOriginal = valuesOriginal.at(3);
	int percentSelected = valuesSelected.at(3);

	vtkStdString tooltipLabel = "Selected: " + std::to_string(percentSelected) + " % \n" +
								"Original: " + std::to_string(percentOriginal) + " %" ;

	m_toolTip->SetText(tooltipLabel);
}

void iACompBarChart::BarChartInteractorStyle::hideTooltip()
{
	m_toolTip->SetVisible(false);
}

void iACompBarChart::BarChartInteractorStyle::OnLeftButtonUp() {}
void iACompBarChart::BarChartInteractorStyle::OnMouseMove() {}
void iACompBarChart::BarChartInteractorStyle::OnMiddleButtonDown() {}
void iACompBarChart::BarChartInteractorStyle::OnRightButtonDown() {}
void iACompBarChart::BarChartInteractorStyle::OnMouseWheelForward() {}
void iACompBarChart::BarChartInteractorStyle::OnMouseWheelBackward() {}
void iACompBarChart::BarChartInteractorStyle::OnKeyPress() {}
void iACompBarChart::BarChartInteractorStyle::OnKeyRelease() {}
