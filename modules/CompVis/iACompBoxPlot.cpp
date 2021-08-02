#include "iACompBoxPlot.h"

//CompVis
#include "iACompVisOptions.h"

//iA
#include "iAMainWindow.h"
#include "iAQVTKWidget.h"

//vtk

#include "vtkRenderer.h"
#include <vtkRenderWindow.h>
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
#include "vtkVariantArray.h"


#include "vtkComputeQuartiles.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkLookupTable.h"
#include "vtkVariant.h"

#include "vtkTextActor.h"
#include "vtkTooltipItem.h"
#include "vtkContextMouseEvent.h"
#include "vtkProperty2D.h"

#include "vtkCellArray.h"
#include "vtkPolyDataMapper.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkRegularPolygonSource.h"
#include "vtkCoordinate.h"
#include "vtkActor2D.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkDataSetAttributes.h"

#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkContext2D.h"

// Qt
#include <QBoxLayout>

#include <string>

vtkStandardNewMacro(iACompBoxPlot::BoxPlotChart);
vtkStandardNewMacro(iACompBoxPlot::BoxPlot);

iACompBoxPlot::iACompBoxPlot(iAMainWindow* parent, iACsvDataStorage* dataStorage) : 
	QDockWidget(parent),
	m_dataStorage(dataStorage),
	m_qvtkWidget(new iAQVTKWidget(this)),
	maxValsAttr(new std::vector<double>()),
	minValsAttr(new std::vector<double>()),
	m_numberOfAttr(0),
	m_legendAttributes(new std::vector<vtkSmartPointer<vtkTextActor>>()),
	labels(vtkSmartPointer<vtkStringArray>::New()),
	currentQuartileTable(vtkSmartPointer<vtkTable>::New()),
	finishedInitalization(false)
	
{
	setupUi(this);

	this->setFeatures(DockWidgetVerticalTitleBar);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	layout->addWidget(m_qvtkWidget);

	m_view = vtkSmartPointer<vtkContextView>::New();

	m_view->SetRenderWindow(m_qvtkWidget->renderWindow());
	m_view->SetInteractor(m_qvtkWidget->interactor());
}

void iACompBoxPlot::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);

	//reset chart if something was drawn before
	m_view->GetScene()->ClearItems();

	for (int i = 0; i < ((int)m_legendAttributes->size()); i++)
	{
		m_view->GetRenderer()->RemoveActor2D(m_legendAttributes->at(i));
	}
	m_legendAttributes->clear();

	//create lookup tables
	initializeLutForOriginalBoxPlot();
	initializeLutForSelectedBoxPlot();

	//create chart (a plot is stored inside a chart)
	m_chartOriginal = vtkSmartPointer<BoxPlotChart>::New();
	m_chartOriginal->setOuterClass(this);
	m_chartOriginal->Update();
	m_view->GetScene()->AddItem(m_chartOriginal);

	//create box plot
	m_boxOriginal = vtkSmartPointer<BoxPlot>::New();
	m_boxOriginal->SetInputData(normalizedTable);
	m_boxOriginal->SetLookupTable(lutOriginal);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);
	m_boxOriginal->SetColor(col[0], col[1], col[2]);
	m_boxOriginal->GetPen()->SetWidth(m_boxOriginal->GetPen()->GetWidth() * 2.5);

	m_chartOriginal->SetPlot(m_boxOriginal);
	m_chartOriginal->SetColumnVisibilityAll(true);
	m_chartOriginal->Update();

	//render all to get position of columns
	renderWidget();

	//set size of chart
	m_chartOriginal->SetPoint1(m_qvtkWidget->width()*0.0, (m_qvtkWidget->height()*0.3));
	m_chartOriginal->SetPoint2(m_qvtkWidget->width()*0.75, (m_qvtkWidget->height())*0.85);
	m_chartOriginal->Update();

	initializeAxes(m_chartOriginal, true);
	initializeLegend(m_chartOriginal);

	finishedInitalization = true;

	this->renderWidget();
}

void iACompBoxPlot::reinitializeBoxPlot()
{
	m_view->GetScene()->ClearItems();
	removeSelectedMessage();

	for(int i = 0; i < ((int)m_legendAttributes->size()); i++)
	{
		m_view->GetRenderer()->RemoveActor2D(m_legendAttributes->at(i));
	}

	m_legendAttributes->clear();
	labels->Initialize();

	// data preparation
	initializeData();

	//create chart (a plot is stored inside a chart)
	m_chartOriginal = vtkSmartPointer<BoxPlotChart>::New();
	m_chartOriginal->setOuterClass(this);
	m_chartOriginal->Update();
	m_view->GetScene()->AddItem(m_chartOriginal);

	//create box plot
	m_boxOriginal = vtkSmartPointer<BoxPlot>::New();
	m_boxOriginal->SetInputData(normalizedTable);
	m_boxOriginal->SetLookupTable(lutOriginal);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);
	m_boxOriginal->SetColor(col[0], col[1], col[2]);
	m_boxOriginal->GetPen()->SetWidth(m_boxOriginal->GetPen()->GetWidth() * 2.5);

	m_chartOriginal->SetPlot(m_boxOriginal);
	m_chartOriginal->SetColumnVisibilityAll(true);
	m_chartOriginal->Update();

	//render all to get position of columns
	renderWidget();

	//set size of chart
	m_chartOriginal->SetPoint1(m_qvtkWidget->width()*0.0, (m_qvtkWidget->height()*0.3));
	m_chartOriginal->SetPoint2(m_qvtkWidget->width()*0.75, (m_qvtkWidget->height())*0.85);
	m_chartOriginal->Update();
	renderWidget();

	initializeAxes(m_chartOriginal, true);
	initializeLegend(m_chartOriginal);

	renderWidget();
}

void iACompBoxPlot::initializeData()
{
	// data preparation
	QList<csvFileData>* data = m_dataStorage->getData();
	QStringList* attrNames = m_dataStorage->getAttributeNamesWithoutLabel();
	m_numberOfAttr = attrNames->size(); //amount of attributes
	
	// Set the labels
	labels->Initialize();

	vtkSmartPointer<vtkTable> originalValuesTable = vtkSmartPointer<vtkTable>::New();
	m_originalOrderTable = vtkSmartPointer<vtkTable>::New();

	//set amount of attributes
	for (int i = 0; i < m_numberOfAttr; i++)
	{
		labels->InsertNextValue(attrNames->at(i).toStdString());

		vtkSmartPointer<vtkDoubleArray> arrIndex1 = vtkSmartPointer<vtkDoubleArray>::New();
		arrIndex1->SetName(attrNames->at(i).toStdString().c_str());

		vtkSmartPointer<vtkDoubleArray> arrIndex2 = vtkSmartPointer<vtkDoubleArray>::New();
		arrIndex2->SetName(attrNames->at(i).toStdString().c_str());
		
		originalValuesTable->AddColumn(arrIndex1);
		m_originalOrderTable->AddColumn(arrIndex2);
	}

	//calculate amount of objects(fibers)/rows
	int numberOfRows = 0;
	for (int i = 0; i < data->size(); i++)
	{
		numberOfRows += csvDataType::getRows(data->at(i).values);
	}
	originalValuesTable->SetNumberOfRows(numberOfRows);
	m_originalOrderTable->SetNumberOfRows(numberOfRows);

	int row = 0;
	//fill table with data
	for (int i = 0; i < data->size(); i++)
	{//for all datasets
		for (int dataInd = 0; dataInd < ((int)data->at(i).values->size()); dataInd++)
		{ //for all values
			for (int attrInd = 1; attrInd <= m_numberOfAttr; attrInd++)

			{//for all attributes but without the label attribute

				int col = attrInd - 1;
				double val = data->at(i).values->at(dataInd).at(m_orderedPositions->at(col) + 1);
				
				originalValuesTable->SetValue(row, col, vtkVariant(val));

				double valOriginalOrder = data->at(i).values->at(dataInd).at(attrInd);
				m_originalOrderTable->SetValue(row, col, valOriginalOrder);
			}

			row++;
		}
	}

	//calculate quartiles
	outTable = calcualteVTKQuartiles(originalValuesTable);
	currentQuartileTable->DeepCopy(outTable);
	currentQuartileTable->Modified();

	m_originalOrderTableNotNormalized = calcualteVTKQuartiles(m_originalOrderTable);
	m_originalOrderTable = normalizeTable(m_originalOrderTableNotNormalized);

	//normalize quartiles in the interval [0,1]
	normalizedTable = normalizeTable(outTable);
}

void iACompBoxPlot::initializeAxes(vtkSmartPointer<BoxPlotChart> chart, bool axesVisibleOn)
{
	//create title
	chart->SetTitle("Interval Similarity");
	chart->GetTitleProperties()->BoldOn();
	chart->GetTitleProperties()->ItalicOff();
	chart->GetTitleProperties()->ShadowOff();
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);
	chart->GetTitleProperties()->SetColor(col);
	chart->GetTitleProperties()->SetFontFamilyToArial();
	chart->GetTitleProperties()->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	chart->GetTitleProperties()->Modified();

	//create y axis
	vtkAxis *axisLeft = chart->GetYAxis();
	axisLeft->SetBehavior(1);
	axisLeft->SetTitle("Converted Interval Size");
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
	
	axisLeft->SetNumberOfTicks(5);
	axisLeft->SetMinimum(0.0);
	axisLeft->SetMaximum(1.0);
	axisLeft->SetNotation(2);
	axisLeft->SetPrecision(2);
	axisLeft->Update();

	if(!axesVisibleOn)
	{
		chart->GetTitleProperties()->SetOpacity(0);
		chart->GetTitleProperties()->Modified();

		axisLeft->GetTitleProperties()->SetOpacity(0);
		axisLeft->GetTitleProperties()->Modified();

		axisLeft->SetTicksVisible(false);
		axisLeft->SetAxisVisible(false);

		axisLeft->GetLabelProperties()->SetOpacity(0);

		axisLeft->SetOpacity(0);
		axisLeft->Update();
	}

	chart->Update();
}

void iACompBoxPlot::initializeLutForOriginalBoxPlot()
{
	lutOriginal = vtkSmartPointer<vtkLookupTable>::New();
	lutOriginal->SetNumberOfColors(m_numberOfAttr);
	lutOriginal->Build();
	
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);

	for(int i = 0; i < m_numberOfAttr; i++)
	{
		lutOriginal->SetTableValue(i, col[0], col[1], col[2], 0);
	}
}

void iACompBoxPlot::initializeLutForSelectedBoxPlot()
{
	lutSelected = vtkSmartPointer<vtkLookupTable>::New();
	lutSelected->SetNumberOfColors(m_numberOfAttr);
	lutSelected->Build();

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);

	for (int i = 0; i < m_numberOfAttr; i++)
	{
		lutSelected->SetTableValue(i, col[0], col[1], col[2], 0.25);
	}
}

void iACompBoxPlot::initializeLegend(vtkSmartPointer<BoxPlotChart> chart)
{
	//for each column draw a legend
	double offset = 1.0;
	double factor;
	
	if (m_numberOfAttr == 1)
	{
		factor = offset;
	}else
	{
		factor = offset / ((double)(m_numberOfAttr));
	}

	offset = offset - factor;
	
	for (int i = 0; i < m_numberOfAttr; i++)
	{
		vtkSmartPointer<vtkTextActor> legend = vtkSmartPointer<vtkTextActor>::New();
		legend->SetTextScaleModeToNone();
		legend->SetInput(labels->GetValue(m_orderedPositions->at(i))); //reordered positions

		vtkSmartPointer<vtkTextProperty> legendProperty = legend->GetTextProperty();
		legendProperty->BoldOff();
		legendProperty->ItalicOff();
		legendProperty->ShadowOff();
		legendProperty->SetFontFamilyToArial();
		legendProperty->SetColor(0, 0, 0);

		if(m_numberOfAttr < 20)
		{
			legendProperty->SetOrientation(20);//use with material data
		}
		else {
			legendProperty->SetOrientation(30);//use with unemployment data
		}		
		
		legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
		legendProperty->SetJustification(VTK_TEXT_RIGHT);
		legendProperty->SetVerticalJustificationToTop();
		legendProperty->Modified();

		float x = chart->GetXPosition(i) + (BoxPlot::SafeDownCast(chart->GetPlot(0))->GetBoxWidth()*offset);
		legend->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
		legend->GetPositionCoordinate()->SetValue(x, m_qvtkWidget->height()*0.27);
		legend->Modified();

		m_view->GetRenderer()->AddActor(legend);
		m_legendAttributes->push_back(legend);

		////TESTING
		//LOG(lvlDebug,"chart->GetXPosition(i) = (" + QString::number(chart->GetXPosition(i)) + ")");
		//LOG(lvlDebug,"(box->GetBoxWidth()) = (" + QString::number(BoxPlot::SafeDownCast(chart->GetPlot(0))->GetBoxWidth() * offset) + ")");

		//vtkSmartPointer<vtkRegularPolygonSource> point = vtkSmartPointer<vtkRegularPolygonSource>::New();
		//point->SetNumberOfSides(50);
		//point->SetRadius(1);
		//point->SetCenter(chart->GetXPosition(i), m_qvtkWidget->height()*0.25, 0);

		//// Visualize
		//vtkSmartPointer<vtkPolyDataMapper2D> mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
		//mapper->SetInputConnection(point->GetOutputPort());

		//vtkSmartPointer<vtkActor2D> actor = vtkSmartPointer<vtkActor2D>::New();
		//actor->SetMapper(mapper);
		//actor->GetProperty()->SetColor(1, 0, 0);
		//m_view->GetRenderer()->AddActor2D(actor);

		offset = offset - factor;
	}
}

void iACompBoxPlot::renderWidget()
{
	m_qvtkWidget->renderWindow()->GetInteractor()->Render();
}

void iACompBoxPlot::setOrderedPositions(std::vector<double>* orderedPositions)
{
	m_orderedPositions = orderedPositions;

	// data preparation
	initializeData();
}

double iACompBoxPlot::median(std::vector<double>::const_iterator begin, std::vector<double>::const_iterator end)
{
	int len = end - begin;
	auto it = begin + len / 2;
	double m = *it;

	if ((len % 2) == 0)
	{
		m = (m + *(--it)) / 2;
	}
	return m;
}

std::vector<double>* iACompBoxPlot::calcualteQuartiles(std::vector<double> v)
{
	std::vector<double>* result = new std::vector<double>();

	//sort input vector!
	std::sort(v.begin(), v.end());

	auto it_second_half = v.cbegin() + v.size() / 2;
	auto it_first_half = it_second_half;
	if ((v.size() % 2) == 0) --it_first_half;

	
	double q1 = median(v.begin(), it_first_half);
	double q2 = median(v.begin(), v.end());
	double q3 = median(it_second_half, v.end());

	result->push_back(q1);
	result->push_back(q2);
	result->push_back(q3);
	return result;
}

vtkSmartPointer<vtkTable> iACompBoxPlot::calcualteVTKQuartiles(vtkSmartPointer<vtkTable> input)
{
	vtkSmartPointer<vtkComputeQuartiles> quartiles = vtkSmartPointer<vtkComputeQuartiles>::New();
	quartiles->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, input);
	quartiles->Update();

	return quartiles->GetOutput();
}

vtkSmartPointer<vtkTable> iACompBoxPlot::normalizeTable(vtkSmartPointer<vtkTable> input)
{
	vtkSmartPointer<vtkTable> normalizedTableCurr = vtkSmartPointer<vtkTable>::New();
	normalizedTableCurr->DeepCopy(input);

	//normalize quartiles in the interval [0,1]
	for (vtkIdType c = 0; c < input->GetNumberOfColumns(); c++)
	{
		for (vtkIdType r = 0; r < input->GetNumberOfRows(); r++)
		{
			vtkVariant v = input->GetValue(r, c);
			double newV = iACompVisOptions::histogramNormalization(v.ToDouble(), 0.0, 1.0, input->GetValue(0, c).ToDouble(), input->GetValue(input->GetNumberOfRows() - 1, c).ToDouble());
			normalizedTableCurr->SetValue(r, c, vtkVariant(newV));
		}
	}

	return normalizedTableCurr;
}

vtkSmartPointer<vtkTable> iACompBoxPlot::normalizeTableSelected(vtkSmartPointer<vtkTable> input, std::vector<double>* selected_orderedPositions)
{
	vtkSmartPointer<vtkTable> normalizedTableCurr = vtkSmartPointer<vtkTable>::New();
	normalizedTableCurr->DeepCopy(input);

	//normalize quartiles in the interval [0,1]
	for (vtkIdType c = 0; c < input->GetNumberOfColumns(); c++)
	{
		for (vtkIdType r = 0; r < input->GetNumberOfRows(); r++)
		{
			double originalMin = m_originalOrderTableNotNormalized->GetValue(0, selected_orderedPositions->at(c)).ToDouble();
			double originalMax = m_originalOrderTableNotNormalized->GetValue(m_originalOrderTableNotNormalized->GetNumberOfRows() - 1, selected_orderedPositions->at(c)).ToDouble();

			vtkVariant v = input->GetValue(r, c);
			double newV = iACompVisOptions::histogramNormalization(v.ToDouble(), 0.0, 1.0, originalMin, originalMax);

			//catch numerical errors for 0
			if (newV - 0.0 < 1e-7) { 
				newV = 0.0;
			};

			//catch numerical errors for 1
			if (1.0 - newV < 1e-7) {
				newV = 1.0;
			};

			normalizedTableCurr->SetValue(r, c, vtkVariant(newV));
		}
	}

	return normalizedTableCurr;
}

/******************************************  Update Methods  **********************************/

void iACompBoxPlot::updateBoxPlot(csvDataType::ArrayType* selectedData, std::vector<double>* selected_orderedPositions)
{	
	//reset box plot
	m_view->GetScene()->RemoveItem(m_chartSelected);
	removeSelectedMessage();

	//reorder original data table
	reorderOriginalData(selected_orderedPositions);

	//create selected data table
	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
	vtkSmartPointer<vtkTable> selectedNormalizedTable = vtkSmartPointer<vtkTable>::New();

	// Set the labels
	QStringList* attrNames = m_dataStorage->getAttributeNamesWithoutLabel();
	labels->Initialize();

	//set amount of attributes
	for (int i = 0; i < m_numberOfAttr; i++)
	{
		labels->InsertNextValue(attrNames->at(i).toStdString());

		vtkSmartPointer<vtkDoubleArray> arrIndex = vtkSmartPointer<vtkDoubleArray>::New();
		arrIndex->SetName(attrNames->at(i).toStdString().c_str());
		table->AddColumn(arrIndex);
	}

	if(selectedData->size() == 0 || selectedData->at(0).size() == 0 || selectedData->at(0).size() == 1 || selectedData->at(0).size() == 2)
	{// no data -> draw empty box plot
		
		drawNotEnoughObjectsSelectedMessage();
		renderWidget();

		return;
	}
	else
	{
		//calculate amount of objects(fibers)/rows
		table->SetNumberOfRows(selectedData->at(0).size());

		//fill table with data
		for (int col = 1; col < ((int)selectedData->size()); col++)
		{//for each attribute

			for (int row = 0; row < ((int)selectedData->at(col).size()); row++)
			{
				int colNew = col - 1;
				table->SetValue(row, colNew, selectedData->at(selected_orderedPositions->at(colNew) + 1).at(row));
			}
		}

		//calculate quartiles
		vtkSmartPointer<vtkTable> quartileTable = calcualteVTKQuartiles(table);
		currentQuartileTable->DeepCopy(quartileTable);
		currentQuartileTable->Modified();

		//normalize quartiles in the interval [0,1]
		selectedNormalizedTable = normalizeTableSelected(quartileTable, selected_orderedPositions);
	}

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);

	//create box plot
	m_boxSelected = vtkSmartPointer<BoxPlot>::New();
	m_boxSelected->setNumberOfColumns(m_numberOfAttr);
	m_boxSelected->setOuterClass(this);
	m_boxSelected->SetInputData(selectedNormalizedTable);
	//color of inner space
	m_boxSelected->SetLookupTable(lutSelected);
	//color of bar contours
	m_boxSelected->SetColor(col[0], col[1], col[2]);
	m_boxSelected->Update();

	m_chartSelected = vtkSmartPointer<BoxPlotChart>::New();
	m_chartSelected->setOuterClass(this);
	m_chartSelected->SetPlot(m_boxSelected);
	m_chartSelected->SetColumnVisibilityAll(true);

	//render all to get position of columns
	renderWidget();

	//set size of chart
	m_chartSelected->SetPoint1(m_qvtkWidget->width()*0.0, (m_qvtkWidget->height()*0.3));
	m_chartSelected->SetPoint2(m_qvtkWidget->width()*0.75, (m_qvtkWidget->height())*0.85);
	m_chartSelected->Update();

	//initialize axes as the original box plot
	initializeAxes(m_chartSelected, false);

	m_view->GetScene()->AddItem(m_chartSelected);

	//reorder label legend
	reorderLegend(selected_orderedPositions);

	renderWidget();
}

void iACompBoxPlot::reorderOriginalData(std::vector<double>* selected_orderedPositions)
{
	//create selected data table
	reorderedNormalizedTable = vtkSmartPointer<vtkTable>::New();

	//set amount of attributes
	for (int i = 0; i < m_numberOfAttr; i++)
	{
		vtkSmartPointer<vtkDoubleArray> arrIndex = vtkSmartPointer<vtkDoubleArray>::New();
		arrIndex->SetName(std::to_string(i).c_str());
		reorderedNormalizedTable->AddColumn(arrIndex);
	}

	reorderedNormalizedTable->SetNumberOfRows(m_originalOrderTable->GetNumberOfRows());

	//fill table with data
	for (int colId = 0; colId < m_originalOrderTable->GetNumberOfColumns(); colId++)
	{//for each attribute

		for (int row = 0; row < m_originalOrderTable->GetNumberOfRows(); row++)
		{
			double v = m_originalOrderTable->GetValue(row, selected_orderedPositions->at(colId)).ToDouble();
			reorderedNormalizedTable->SetValue(row, colId, v);
		}
	}

	m_boxOriginal->SetInputData(reorderedNormalizedTable);
	m_boxOriginal->Modified();

	m_chartOriginal->SetPlot(m_boxOriginal);
	m_chartOriginal->SetColumnVisibilityAll(true);
	m_chartOriginal->Update();

	m_chartOriginal->SetPoint1(m_qvtkWidget->width()*0.0, (m_qvtkWidget->height()*0.3));
	m_chartOriginal->SetPoint2(m_qvtkWidget->width()*0.75, (m_qvtkWidget->height())*0.85);
	m_chartOriginal->Update();
}

void iACompBoxPlot::reorderLegend(std::vector<double>* selected_orderedPositions)
{
	for (int i = 0; i < ((int)m_legendAttributes->size()); i++)
	{
		vtkSmartPointer<vtkTextActor> legend = m_legendAttributes->at(i);
		legend->SetInput(labels->GetValue(selected_orderedPositions->at(i)));
		legend->Modified();
	}
}

void iACompBoxPlot::resetBoxPlot()
{
	//reset box plot
	m_view->GetScene()->RemoveItem(m_chartSelected);
	removeSelectedMessage();

	// Set the labels
	QStringList* attrNames = m_dataStorage->getAttributeNamesWithoutLabel();
	vtkSmartPointer<vtkStringArray> labels = vtkSmartPointer<vtkStringArray>::New();

	for (int i = 0; i < m_numberOfAttr; i++)
	{
		labels->InsertNextValue(attrNames->at(i).toStdString());
	}

	currentQuartileTable->DeepCopy(outTable);
	currentQuartileTable->Modified();

	//set data
	m_boxOriginal->SetInputData(normalizedTable);
	m_boxOriginal->Modified();

	m_chartOriginal->SetPlot(m_boxOriginal);
	m_chartOriginal->SetColumnVisibilityAll(true);
	m_chartOriginal->Update();


	m_chartOriginal->SetPoint1(m_qvtkWidget->width()*0.0, (m_qvtkWidget->height()*0.3));
	m_chartOriginal->SetPoint2(m_qvtkWidget->width()*0.75, (m_qvtkWidget->height())*0.85);
	m_chartOriginal->Update();

	for (int i = 0; i < ((int)m_legendAttributes->size()); i++)
	{
		vtkSmartPointer<vtkTextActor> legend = m_legendAttributes->at(i);
		legend->SetInput(labels->GetValue(m_orderedPositions->at(i)));
		legend->Modified();
	}

	renderWidget();
}

void iACompBoxPlot::updateLegend()
{
	double offset = 1.0;
	double factor = offset / ((double)(m_numberOfAttr - 1));

	for (int i = 0; i < m_numberOfAttr; i++)
	{
		float x = m_chartOriginal->GetXPosition(i) + (m_boxOriginal->GetBoxWidth()*offset);
		vtkSmartPointer<vtkTextActor> legend = m_legendAttributes->at(i);
		legend->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
		legend->GetPositionCoordinate()->SetValue(x, m_qvtkWidget->height()*0.27);

		legend->Modified();

		offset = offset - factor;
	}
}

void iACompBoxPlot::drawNotEnoughObjectsSelectedMessage()
{
	notEnoughElementsSelectedTextActor = vtkSmartPointer<vtkTextActor>::New();
	notEnoughElementsSelectedTextActor->SetTextScaleModeToNone();
	notEnoughElementsSelectedTextActor->SetInput("Not enough objects selected to calculate a box plot.");

	vtkSmartPointer<vtkTextProperty> property = notEnoughElementsSelectedTextActor->GetTextProperty();
	property->BoldOn();
	property->ItalicOff();
	property->ShadowOff();
	property->SetFontFamilyToArial();
	property->SetColor(0, 0, 0);
	property->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	property->SetJustification(VTK_TEXT_CENTERED);
	property->SetVerticalJustificationToTop();
	property->Modified();

	notEnoughElementsSelectedTextActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
	notEnoughElementsSelectedTextActor->GetPositionCoordinate()->SetValue(0.5, 0.5);
	notEnoughElementsSelectedTextActor->Modified();

	m_view->GetRenderer()->AddActor(notEnoughElementsSelectedTextActor);
}

void iACompBoxPlot::removeSelectedMessage()
{
	m_view->GetRenderer()->RemoveActor(notEnoughElementsSelectedTextActor);
}

/************************* INNER CLASS BoxPlotChart *******************************************/
void iACompBoxPlot::BoxPlotChart::SetTooltipInfo(const vtkContextMouseEvent& mouse,
	const vtkVector2d &plotPos,
	vtkIdType seriesIndex, vtkPlot* plot,
	vtkIdType segmentIndex)
{
	if (!this->Tooltip)
	  {
		return;
	  }

	if (seriesIndex == -1 && plot == nullptr && segmentIndex == -1)
	  {
		return;
	  }
	
	std::vector<std::string> tokens;
	
	for (vtkIdType r = 0; r < m_outerClass->currentQuartileTable->GetNumberOfRows(); r++)
	{
		vtkVariant v = m_outerClass->currentQuartileTable->GetValue(r, plotPos.GetX());
		tokens.push_back(v.ToString());
	}

	vtkStdString tooltipLabel = 
		"Minimum = " + tokens.at(0) + "\n" +
		"First quartile = " + tokens.at(1) + "\n" + 
		"Median = " + tokens.at(2) + "\n" + 
		"Third quartile = " + tokens.at(3) + "\n" + 
		"Maximum = " + tokens.at(4);

  // Set the tooltip
  this->Tooltip->SetText(tooltipLabel);
  this->Tooltip->SetPosition(mouse.GetScreenPos()[0] + 2, mouse.GetScreenPos()[1] + 2);
}

void iACompBoxPlot::BoxPlotChart::Update()
{
	if (m_outerClass->finishedInitalization)
	{
		this->SetPoint1(m_outerClass->m_qvtkWidget->width()*0.0, (m_outerClass->m_qvtkWidget->height()*0.3));
		this->SetPoint2(m_outerClass->m_qvtkWidget->width()*0.75, (m_outerClass->m_qvtkWidget->height())*0.85);
	}
	
	vtkChartBox::Update();
}

void iACompBoxPlot::BoxPlotChart::setOuterClass(iACompBoxPlot * outerClass)
{
	m_outerClass = outerClass;
}

iACompBoxPlot::BoxPlotChart::BoxPlotChart()
{
}

/************************* INNER CLASS vtkPlotBox *******************************************/

class vtkPlotBox::Private : public std::vector<std::vector<double>>
{
public:
	Private() = default;
};

iACompBoxPlot::BoxPlot::BoxPlot() {}

void iACompBoxPlot::BoxPlot::setOuterClass(iACompBoxPlot * outerClass)
{
	m_outerClass = outerClass;
}

void iACompBoxPlot::BoxPlot::setNumberOfColumns(int nmbCols)
{
	m_numberOfColumns = nmbCols;
}

bool iACompBoxPlot::BoxPlot::Paint(vtkContext2D *painter)
{
	// This is where everything should be drawn, or dispatched to other methods.
	vtkDebugMacro(<< "Paint event called in vtkPlotBox.");

	if (!this->Visible)
	{
		return false;
	}

	if (this->Storage->empty() || this->Storage->at(0).size() != 5)
	{
		vtkErrorMacro(
			<< "Input table must contain 5 rows per column. These rows hold min, quartile 1, median, "
			"quartile 2 and max. Use vtkComputeQuartiles to create a proper table.");
		return false;
	}

	vtkChartBox* parent = vtkChartBox::SafeDownCast(this->Parent);

	int nbCols = static_cast<int>(this->Storage->size());


	for (int i = 0; i < nbCols; i++)
	{
		vtkStdString colName = parent->GetVisibleColumns()->GetValue(i);
		int index;
		this->GetInput()->GetRowData()->GetAbstractArray(colName.c_str(), index);
		double rgb[4];
		this->LookupTable->GetIndexedColor(index, rgb);

		unsigned char crgba[4] = { static_cast<unsigned char>(rgb[0] * 255.),
		  static_cast<unsigned char>(rgb[1] * 255.), static_cast<unsigned char>(rgb[2] * 255.), static_cast<unsigned char>(rgb[3] * 255.) };

		if (parent->GetSelectedColumn() == i)
		{
			crgba[0] = crgba[0] ^ 255;
			crgba[1] = crgba[1] ^ 255;
			crgba[2] = crgba[2] ^ 255;
			crgba[3] = crgba[3] ^ 255;
		}

		DrawBoxPlot(i, crgba, parent->GetXPosition(i), painter);
	}

	return true;
}

void iACompBoxPlot::BoxPlot::DrawBoxPlot(int i, unsigned char* rgba, double x, vtkContext2D* painter)
{
	std::vector<double>& colQuartiles = this->Storage->at(i);
	if (colQuartiles.size() < 5)
	{
		return;
	}
	painter->ApplyPen(this->Pen);

	vtkNew<vtkBrush> brush;
	brush->SetColor(rgba);

	brush->SetOpacityF( (rgba[3]/ 255.)); //new

	painter->ApplyBrush(brush.GetPointer());

	// Helper variables for x position
	double xpos = x + 0.5 * this->BoxWidth;
	double xneg = x - 0.5 * this->BoxWidth;
	double hBoxW = this->BoxWidth * 0.25;

	// Fetch the quartiles and median
	double* q = &colQuartiles[0];

	// Draw the box
	painter->DrawQuad(xpos, q[1], xneg, q[1], xneg, q[3], xpos, q[3]);

	// Draw the whiskers: ends of the whiskers match the
	// extremum values of the quartiles
	painter->DrawLine(x, q[0], x, q[1]);
	painter->DrawLine(x - hBoxW, q[0], x + hBoxW, q[0]);
	painter->DrawLine(x, q[3], x, q[4]);
	painter->DrawLine(x - hBoxW, q[4], x + hBoxW, q[4]);

	// Draw the median
	vtkNew<vtkPen> whitePen;
	unsigned char brushColor[4];
	brush->GetColor(brushColor);
	// Use a gray pen if the brush is black so the median is always visible
	if (brushColor[0] == 0 && brushColor[1] == 0 && brushColor[2] == 0)
	{
		whitePen->SetWidth(this->Pen->GetWidth());
		whitePen->SetColor(128, 128, 128, 128);
		whitePen->SetOpacity(this->Pen->GetOpacity());
		painter->ApplyPen(whitePen.GetPointer());
	}

	painter->DrawLine(xneg, q[2], xpos, q[2]);
}
