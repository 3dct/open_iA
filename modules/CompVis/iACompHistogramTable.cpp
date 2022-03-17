/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iACompHistogramTable.h"

//Debug
//#include "iALog.h"

//CompVis
#include "iACompVisOptions.h"
#include "iACompVisMain.h"

//iA
#include <iAMainWindow.h>
#include <iAQVTKWidget.h>

//Qt
#include <QColor>
#include <QVBoxLayout>

//vtk
#include <vtkActor.h>

#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkLookupTable.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>

#include <vtkColorTransferFunction.h>
#include <vtkNamedColors.h>
#include <vtkOutlineFilter.h>

#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>
#include <vtkUnsignedCharArray.h>

#include <QVTKInteractor.h>
#include <vtkActorCollection.h>
#include <vtkCamera.h>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkLineSource.h>
#include <vtkPolyLine.h>
#include <vtkRegularPolygonSource.h>
#include <vtkSphereSource.h>
#include <vtkGlyph2D.h>

#include <vtkBillboardTextActor3D.h>
#include <vtkCaptionActor2D.h>
#include <vtkFollower.h>
#include <vtkTextActor.h>
#include <vtkTextRepresentation.h>
#include <vtkTextWidget.h>
#include <vtkVectorText.h>

#include <vtkCamera.h>

#include <vtkDataSetMapper.h>
#include <vtkExtractSelection.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkTriangleFilter.h>
#include <vtkUnstructuredGrid.h>

#include <vtkShrinkPolyData.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <functional>
#include <limits>
#include <tuple>
#include <vector>


iACompHistogramTable::iACompHistogramTable(
	iAMainWindow* parent, iAMultidimensionalScaling* mds, iACsvDataStorage* dataStorage, iACompVisMain* main) :
	QDockWidget(parent),
	m_main(main),
	m_dataStorage(dataStorage),
	m_datasetNameActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_inputData(mds->getCSVFileData()),
	m_mds(mds),
	m_histData(nullptr),
	m_qvtkWidget(new iAQVTKWidget(this)),
	m_renderer(vtkSmartPointer<vtkRenderer>::New()),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_tableSize(10),
	m_pointRepresentationActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_originalPlaneActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_zoomedPlaneActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	originalPlaneZoomedPlanePair(new std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>*>()),
	m_highlighingActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_pickedCellsforPickedRow(nullptr),
	m_indexOfPickedRow(nullptr),
	m_zoomedRowData(nullptr),
	m_orderOfIndicesDatasets(nullptr),
	m_rowDataIndexPair(new std::map<vtkSmartPointer<vtkActor>, int>()),
	m_originalOrderOfIndicesDatasets(nullptr),
	m_drawingPositionForRegions(new std::map<int, std::vector<double>>()),
	m_highlightRowActor(vtkSmartPointer<vtkActor>::New()),
	m_useDarkerLut(false),
	m_lutDarker(vtkSmartPointer<vtkLookupTable>::New()),
	m_barActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_barTextActors(new std::vector<vtkSmartPointer<vtkTextActor>>()),
	m_stippledActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	
	m_initialRendering(true),
	m_oldDrawingPosition(-1),
	m_newDrawingPosition(-1),
	m_BinRangeLength(0)
{
	//initialize GUI
	setupUi(this);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	layout->addWidget(m_qvtkWidget);

	std::vector<int>* dataResolution = csvFileData::getAmountObjectsEveryDataset(m_inputData);
	m_amountDatasets = dataResolution->size();
	m_orderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);
	m_originalOrderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);
	m_newOrderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);

	m_bins = minBins;
	m_binsZoomed = minBins;
	m_colSize = 0.0;
	m_rowSize = 0.0;
	screenRatio = 0.0;
	m_windowWidth = 0.0;
	m_windowHeight = 0.0;
	//initialize datastructure
	calculateHistogramTable();

	//setup rendering environment
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	m_renderer->SetBackground(col);
	m_renderer->SetViewport(0, 0, 0.8, 1);
	m_renderer->SetUseFXAA(true);

	m_qvtkWidget->renderWindow()->AddRenderer(m_renderer);
}

void iACompHistogramTable::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);
	
	if(m_initialRendering)
	{
		m_windowWidth = (double)m_qvtkWidget->width();
		m_windowHeight = (double)m_qvtkWidget->height();

		calculateRowWidthAndHeight(m_windowWidth, m_windowHeight, m_amountDatasets);
		m_initialRendering = false;
	}

	//initialize drawing areas of rows for manual repositioning
	determineRowAreas();

	//initialize visualization
	initializeHistogramTable();
}

void iACompHistogramTable::reinitializeHistogramTable(iAMultidimensionalScaling* newMds)
{
	m_mds = newMds;
	m_inputData = m_mds->getCSVFileData();

	m_BinRangeLength = 0;
	m_lut = vtkSmartPointer<vtkLookupTable>::New();
	m_tableSize = 10;

	m_pointRepresentationActors->clear();
	delete m_pointRepresentationActors;
	m_pointRepresentationActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_datasetNameActors->clear();
	delete m_datasetNameActors;
	m_datasetNameActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_highlighingActors->clear();
	delete m_highlighingActors;
	m_highlighingActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_originalPlaneActors->clear();
	delete m_originalPlaneActors;
	m_originalPlaneActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_zoomedPlaneActors->clear();
	delete m_zoomedPlaneActors;
	m_zoomedPlaneActors = new std::vector<vtkSmartPointer<vtkActor>>();

	originalPlaneZoomedPlanePair->clear();
	delete originalPlaneZoomedPlanePair;
	originalPlaneZoomedPlanePair = new std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>*>();
	
	m_rowDataIndexPair->clear();
	delete m_rowDataIndexPair;
	m_rowDataIndexPair = new std::map<vtkSmartPointer<vtkActor>, int>();
	
	m_drawingPositionForRegions->clear();
	delete m_drawingPositionForRegions;
	m_drawingPositionForRegions = new std::map<int, std::vector<double>>();
	
	m_highlightRowActor = vtkSmartPointer<vtkActor>::New();
	
	m_useDarkerLut = false;
	
	m_lutDarker = vtkSmartPointer<vtkLookupTable>::New();

	m_barActors->clear();
	delete m_barActors;
	m_barActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_barTextActors->clear();
	delete m_barTextActors;
	m_barTextActors = new std::vector<vtkSmartPointer<vtkTextActor>>();

	m_stippledActors->clear();
	delete m_stippledActors;
	m_stippledActors = new std::vector<vtkSmartPointer<vtkActor>>();

	std::vector<int>* dataResolution = csvFileData::getAmountObjectsEveryDataset(m_inputData);
	m_amountDatasets = dataResolution->size();

	m_orderOfIndicesDatasets->clear();
	delete m_orderOfIndicesDatasets;
	m_orderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);

	m_originalOrderOfIndicesDatasets->clear();
	delete m_originalOrderOfIndicesDatasets;
	m_originalOrderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);

	m_newOrderOfIndicesDatasets->clear();
	delete m_newOrderOfIndicesDatasets;
	m_newOrderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);

	m_bins = minBins;
	m_binsZoomed = minBins;

	//initialize datastructure
	calculateHistogramTable();

	m_qvtkWidget->renderWindow()->RemoveRenderer(m_renderer);

	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	double col1[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col1);
	m_renderer->SetBackground(col1);
	m_renderer->SetViewport(0, 0, 0.8, 1);
	m_renderer->SetUseFXAA(true);

	m_qvtkWidget->renderWindow()->AddRenderer(m_renderer);

	calculateRowWidthAndHeight(m_windowWidth, m_windowHeight, m_amountDatasets);

	//initialize drawing areas of rows for manual repositioning
	determineRowAreas();

	//initialize visualization
	initializeHistogramTable();

}

void iACompHistogramTable::calculateRowWidthAndHeight(double width, double heigth, double numberOfDatasets)
{
	if (heigth > width)
	{
		screenRatio = width / heigth;
		m_colSize = 1;
		m_rowSize = (screenRatio / numberOfDatasets);
	}
	else
	{
		screenRatio = heigth / width;
		m_colSize = (screenRatio / numberOfDatasets);
		m_rowSize = 1;
	}
}

void iACompHistogramTable::determineRowAreas()
{
	//drawing position 0 = bottom until m_amountDatasets-1 = top
	for(int drawingPosition = 0; drawingPosition < m_amountDatasets; drawingPosition++)
	{
		double ymin = m_colSize * drawingPosition;
		double ymax = ymin + m_colSize;
		
		std::vector<double> yInterval = std::vector<double>(2, 0);
		yInterval.at(0) = ymin;
		yInterval.at(1) = ymax;

		m_drawingPositionForRegions->insert({drawingPosition, yInterval});
	}
}

/******************************************  Ordering/Ranking  **********************************/

void iACompHistogramTable::drawHistogramTableAccordingToSimilarity(vtkSmartPointer<vtkActor> referenceData)
{
	//get for all dataset each bin with its MDS values
	QList<bin::BinType*>* binData = m_histData->getBinData();

	//set dataIndex
	std::map<vtkSmartPointer<vtkActor>, int>::iterator it = m_rowDataIndexPair->find(referenceData);
	if (it == m_rowDataIndexPair->end()) { return; }
	int dataIndex = it->second;

	bin::BinType* referenceRow = binData->at(dataIndex);
	std::vector<double> results = std::vector<double>(m_amountDatasets, 0);

	for(int currInd = 0; currInd < m_amountDatasets; currInd++)
	{
		if (currInd == dataIndex)
		{ //there is no difference in the same dataset
			results.at(currInd) = 0.0;

		}else
		{
			bin::BinType* testRow = binData->at(currInd);
			double result = calculateChiSquaredMetric(testRow, referenceRow);
			results.at(currInd) = result;
		}
	}

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = sortWithMemory(results, 0);
	m_orderOfIndicesDatasets = reorderAccordingTo(newOrder);

	drawHistogramTable(m_bins);

	//highlight row which was used as reference
	highlightSelectedRow(m_originalPlaneActors->at(m_amountDatasets-1));
}

double iACompHistogramTable::calculateChiSquaredMetric(bin::BinType* observedFrequency, bin::BinType* expectedFrequency)
{
	double chiSquare = 0.0;

	for(int i = 0; i < ((int)observedFrequency->size()); i++)
	{//for each bin

		if(expectedFrequency->at(i).size() != 0)
		{
			int diff = observedFrequency->at(i).size() - expectedFrequency->at(i).size();
			chiSquare += std::pow(((double)diff), 2) / ((double)expectedFrequency->at(i).size());
		}else
		{
			int diff = observedFrequency->at(i).size();
			chiSquare += std::pow(((double)diff), 2);
		}
	}

	return chiSquare;
}

void iACompHistogramTable::drawHistogramTableAccordingToCellSimilarity(Pick::PickedMap* m_picked)
{
	//get for all dataset each bin with its MDS values
	QList<bin::BinType*>* binData = m_histData->getBinData();
	vtkSmartPointer<vtkActor> referenceData;
	std::vector<vtkIdType>* indexOfCells = new std::vector<vtkIdType>();

	//is only run once, since the map can only contain 1 actor with its selected cells
	for (Pick::PickedMap::iterator it = m_picked->begin(); it != m_picked->end(); ++it)
	{
		referenceData = it->first;
		indexOfCells = it->second;
	}

	//set dataIndex
	std::map<vtkSmartPointer<vtkActor>, int>::iterator it = m_rowDataIndexPair->find(referenceData);
	if (it == m_rowDataIndexPair->end()) { return; }
	int dataIndex = it->second;

	//get all the bins
	bin::BinType* referenceRow = binData->at(dataIndex);
	bin::BinType* referenceCells = bin::copyCells(referenceRow, indexOfCells);

	std::vector<double> results = std::vector<double>(m_amountDatasets, 0);

	for (int currInd = 0; currInd < m_amountDatasets; currInd++)
	{
		if (currInd == dataIndex)
		{ //there is no difference in the same dataset
			results.at(currInd) = 0.0;

		}
		else
		{
			bin::BinType* testRow = binData->at(currInd);
			bin::BinType* testCells = bin::copyCells(testRow, indexOfCells);
			double result = calculateChiSquaredMetric(testCells, referenceCells);
			results.at(currInd) = result;
		}
	}

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = sortWithMemory(results, 0);
	m_orderOfIndicesDatasets = reorderAccordingTo(newOrder);

	drawHistogramTable(m_bins);

	//highlight selected cells
	for(int i= 0; i < ((int)indexOfCells->size()); i++)
	{
		highlightSelectedCell(m_originalPlaneActors->at(m_amountDatasets - 1), indexOfCells->at(i));
	}
}

void iACompHistogramTable::drawHistogramTableInOriginalOrder()
{
	if (m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	iACompVisOptions::copyVector(m_originalOrderOfIndicesDatasets, m_orderOfIndicesDatasets);
	m_useDarkerLut = true;

	drawHistogramTable(m_bins);

	std::vector<int> amountObjectsEveryDataset = *csvFileData::getAmountObjectsEveryDataset(m_inputData);
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
}

void iACompHistogramTable::drawHistogramTableInDescendingOrder()
{
	std::vector<int> amountObjectsEveryDataset = *csvFileData::getAmountObjectsEveryDataset(m_inputData);

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = sortWithMemory(amountObjectsEveryDataset, 1);
	m_orderOfIndicesDatasets = reorderAccordingTo(newOrder);

	if(m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	m_useDarkerLut = true;

	drawHistogramTable(m_bins);
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
}

void iACompHistogramTable::drawHistogramTableInAscendingOrder()
{
	std::vector<int> amountObjectsEveryDataset = *csvFileData::getAmountObjectsEveryDataset(m_inputData);

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = sortWithMemory(amountObjectsEveryDataset, 0);
	m_orderOfIndicesDatasets = reorderAccordingTo(newOrder);

	if (m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	m_useDarkerLut = true;

	drawHistogramTable(m_bins);
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
}

std::vector<int>* iACompHistogramTable::reorderAccordingTo(std::vector<int>* newPositions)
{
	std::vector<int>* result = new std::vector<int>(newPositions->size(), 0);

	for(int i = 0; i < ((int)newPositions->size()); i++)
	{
		result->at((newPositions->size()-1) - i) = newPositions->at(i);
	}

	return result;
}

std::vector<int>* iACompHistogramTable::sortWithMemory(std::vector<int> input, int orderStyle)
{
	std::vector<int>* newOrder = new std::vector<int>(input.size(), 0);
	int n(0);
	std::generate(std::begin(*newOrder), std::end(*newOrder), [&] { return n++; });

	if(orderStyle == 0)
	{ // ascending ordering
		auto comparator = [input](int i1, int i2) { return input.at(i1) < input.at(i2); };
		std::sort(std::begin(*newOrder), std::end(*newOrder), comparator);

		std::sort(std::begin(input), std::end(input), std::less<int>());
	}
	else
	{// descending ordering
		auto comparator = [input](int i1, int i2) { return input.at(i1) > input.at(i2); };
		std::sort(std::begin(*newOrder), std::end(*newOrder), comparator);

		std::sort(std::begin(input), std::end(input), std::greater<int>());
	}
	
	return newOrder;
}

std::vector<int>* iACompHistogramTable::sortWithMemory(std::vector<double> input, int orderStyle)
{
	std::vector<int>* newOrder = new std::vector<int>(input.size(), 0.0);
	int n(0);
	std::generate(std::begin(*newOrder), std::end(*newOrder), [&] { return n++; });

	if (orderStyle == 0)
	{ //ascending ordering
		auto comparator = [input](double i1, double i2) { return input.at(i1) < input.at(i2); };
		std::sort(std::begin(*newOrder), std::end(*newOrder), comparator);

		std::sort(std::begin(input), std::end(input), std::less<double>());
	}
	else
	{
		// descending ordering
		auto comparator = [input](double i1, double i2) { return input.at(i1) > input.at(i2); };
		std::sort(std::begin(*newOrder), std::end(*newOrder), comparator);

		std::sort(std::begin(input), std::end(input), std::greater<double>());
	}

	return newOrder;
}

void iACompHistogramTable::drawReorderedHistogramTable()
{
	iACompVisOptions::copyVector(m_newOrderOfIndicesDatasets, m_orderOfIndicesDatasets);
	drawHistogramTable(m_bins);

	//highlight moved actor
	highlightSelectedRow(m_originalPlaneActors->at(m_newDrawingPosition));
}

void iACompHistogramTable::drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset)
{
	//define bars
	auto minMax = std::minmax_element(begin(amountObjectsEveryDataset), end(amountObjectsEveryDataset));
	int max = *minMax.second;

	for(int i = 0; i < ((int)m_originalPlaneActors->size()); i++)
	{
		vtkSmartPointer<vtkActor> currAct = m_originalPlaneActors->at(i);
		vtkSmartPointer<vtkAlgorithm> currAlgorithm = currAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
		vtkSmartPointer<vtkPlaneSource> currPlane = vtkPlaneSource::SafeDownCast(currAlgorithm);

		createBar(currPlane, amountObjectsEveryDataset.at(m_orderOfIndicesDatasets->at(i)), max);
		createAmountOfObjectsText(currPlane, amountObjectsEveryDataset.at(m_orderOfIndicesDatasets->at(i)));
	}

	renderWidget();
}

void iACompHistogramTable::createAmountOfObjectsText(vtkSmartPointer<vtkPlaneSource> currPlane, int currAmountObjects)
{
	vtkSmartPointer<vtkTextActor> legend = vtkSmartPointer<vtkTextActor>::New();
	legend->SetTextScaleModeToNone();
	legend->SetInput(std::to_string(currAmountObjects).c_str());

	vtkSmartPointer<vtkTextProperty> legendProperty = legend->GetTextProperty();
	legendProperty->BoldOn();
	legendProperty->ItalicOff();
	legendProperty->ShadowOff();
	legendProperty->SetFontFamilyToArial();
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE,col);
	legendProperty->SetColor(col);
	legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	legendProperty->SetJustification(VTK_TEXT_LEFT);
	legendProperty->SetVerticalJustificationToCentered();
	legendProperty->Modified();

	double x = currPlane->GetPoint1()[0] + (m_rowSize * 0.05);
	double height = currPlane->GetPoint2()[1] - currPlane->GetOrigin()[1];
	double y = currPlane->GetOrigin()[1] + (height * 0.5);
	legend->GetPositionCoordinate()->SetCoordinateSystemToWorld();
	legend->GetPositionCoordinate()->SetValue(x,y);
	legend->Modified();

	m_renderer->AddActor(legend);
	m_barTextActors->push_back(legend);
}

void iACompHistogramTable::createBar(vtkSmartPointer<vtkPlaneSource> currPlane, int currAmountObjects, int maxAmountObjects)
{
	//calculate height of bar
	double maxHeight = currPlane->GetPoint2()[1] - currPlane->GetOrigin()[1];
	double height25 = currPlane->GetOrigin()[1] + (maxHeight*0.25);
	double height75 = currPlane->GetOrigin()[1] + (maxHeight*0.75);

	//calculate width of bar
	double maxWidth = currPlane->GetPoint1()[0] - currPlane->GetOrigin()[0];
	double percent = ((double)currAmountObjects) / ((double)maxAmountObjects);
	double correctWidth = maxWidth * percent;
	double correctX = (currPlane->GetOrigin()[0]) + correctWidth;

	vtkSmartPointer<vtkPlaneSource> barPlane = vtkSmartPointer<vtkPlaneSource>::New();
	barPlane->SetXResolution(1);
	barPlane->SetYResolution(1);
	barPlane->SetOrigin(currPlane->GetOrigin()[0], height25, currPlane->GetOrigin()[2]);
	barPlane->SetPoint1(correctX, height25, currPlane->GetPoint1()[2]);
	barPlane->SetPoint2(currPlane->GetPoint2()[0], height75,currPlane->GetPoint2()[2]);
	barPlane->Update();

	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(barPlane->GetOutputPort());
	mapper->Update();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	double color[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, color);
	actor->GetProperty()->SetColor(color[0], color[1], color[2]);

	m_renderer->AddActor(actor);
	m_barActors->push_back(actor);
}

void iACompHistogramTable::removeBarCharShowingAmountOfObjects()
{
	m_useDarkerLut = false;

	for(int i = 0; i < ((int)m_barActors->size()); i++)
	{
		m_renderer->RemoveActor(m_barActors->at(i));
		m_renderer->RemoveActor2D(m_barTextActors->at(i));
	}

	m_barActors->clear();
	m_barTextActors->clear();
}

bool iACompHistogramTable::getBarChartAmountObjectsActive()
{
	return m_useDarkerLut;
}

/******************************************  Rendering  **********************************************/
void iACompHistogramTable::drawHistogramTable(int bins)
{
	calculateRowWidthAndHeight(m_windowWidth, m_windowHeight, m_amountDatasets);

	if (m_renderer->GetViewProps()->GetNumberOfItems() > 0)
	{
		m_renderer->RemoveAllViewProps();
	}
	m_originalPlaneActors->clear();
	m_zoomedPlaneActors->clear();
	m_rowDataIndexPair->clear();

	//draw cells from bottom to top --> so start with last dataset and go to first
	for (int currCol = 0; currCol < m_amountDatasets; currCol++)
	{
		int dataInd = m_orderOfIndicesDatasets->at(currCol);
		drawRow(dataInd, currCol, bins, 0);
	}

	renderWidget();
}

void iACompHistogramTable::drawLinearZoom(Pick::PickedMap* map, int notSelectedBinNumber, int selectedBinNumber, QList<bin::BinType*>* zoomedRowData)
{
	QList<bin::BinType*>* thisZoomedRowData = bin::DeepCopy(zoomedRowData);

	//draw zoomed histogram table new
	m_renderer->RemoveAllViewProps();
	m_originalPlaneActors->clear();
	m_zoomedPlaneActors->clear();
	originalPlaneZoomedPlanePair->clear();
	m_rowDataIndexPair->clear();

	int currCol = 0;
	double offset = 0;
	double distance = 0.03; //distance to other rows in percent [0,1]
	double distanceToParent = 0.015; //distance to row from which it is the zoom representation in percent [0,1]
	bool addedRow = false;
	bool offsetAlreadyAdded = false;

	double newHeight = m_windowHeight - (((m_windowHeight*distance) * 4)*map->size()) - (((m_windowHeight*distanceToParent))*map->size());
	calculateRowWidthAndHeight(m_windowWidth, newHeight, m_amountDatasets+map->size());

	std::vector<vtkSmartPointer<vtkPlaneSource>>* zoomedPlanes = nullptr;
	vtkSmartPointer<vtkPlaneSource> originalPlane;

	for (int counter = 0; counter < m_amountDatasets; counter++)
	{
		int indData = m_orderOfIndicesDatasets->at(counter);

		//check for additional row
		std::vector<int>::iterator it = std::find(m_indexOfPickedRow->begin(), m_indexOfPickedRow->end(), indData);
		if (it != m_indexOfPickedRow->end())
		{
			if (currCol != 0 && !offsetAlreadyAdded) 
			{//do not add a offset when the zoomed row is the undermost
				offset = offset + distance;
			}
			
			//draw zoomed dataset
			std::map<int, std::vector<vtkIdType>*>::const_iterator pos = m_pickedCellsforPickedRow->find(indData);
			std::vector<vtkIdType>* cellIds = pos->second;
			zoomedPlanes = drawZoomedRow(/*indData, */currCol, selectedBinNumber, thisZoomedRowData->last(), offset, cellIds);
			thisZoomedRowData->removeLast();

			currCol += 1;
			addedRow = true;
			offset = offset + distanceToParent + (m_colSize*0.5); // zoomed rows are 2*m_colSize high
		}
		offsetAlreadyAdded = false;

		//draw original datasets
		originalPlane = drawRow(indData, currCol, notSelectedBinNumber, offset);

		if (addedRow)
		{
			//do for each cell of the selected row
			std::map<int, std::vector<vtkIdType>*>::const_iterator pos = m_pickedCellsforPickedRow->find(indData);
			if (pos != m_pickedCellsforPickedRow->end()) {
				std::vector<vtkIdType>* cellIds = pos->second;
				vtkSmartPointer<vtkActor> thisAcc = m_renderer->GetActors()->GetLastActor();
				
				for (int i = 0; i < ((int)cellIds->size()); i++)
				{//highlight each cell of the selected row
					
					highlightSelectedCell(thisAcc, cellIds->at(i));
				}

				//draw line border
				drawLineBetweenRowAndZoomedRow(zoomedPlanes, originalPlane, cellIds);
			}

			offset = offset + distance;
			addedRow = false;
			offsetAlreadyAdded = true;

			//store for each original row its zoomed row actors
			std::vector<vtkSmartPointer<vtkActor>> * zoomedActorsForThisRow = new std::vector<vtkSmartPointer<vtkActor>>();
			for (int i = (zoomedPlanes->size() - 1); i >= 0 ; i--)
			{
				zoomedActorsForThisRow->push_back(m_zoomedPlaneActors->at((m_zoomedPlaneActors->size()-1) - i));
			}
			originalPlaneZoomedPlanePair->insert({ m_originalPlaneActors->at(m_originalPlaneActors->size() - 1), zoomedActorsForThisRow });
		}
		else
		{
			vtkSmartPointer<vtkActor> thisAcc = m_renderer->GetActors()->GetLastActor();
			//set opacity for not selected original rows
			thisAcc->GetProperty()->SetOpacity(0.6);
		}

		currCol += 1;
	}

	
	renderWidget();	
}

vtkSmartPointer<vtkPlaneSource> iACompHistogramTable::drawRow(int currDataInd, int currentColumn, int amountOfBins, double offset)
{
	vtkSmartPointer<vtkPlaneSource> aPlane = vtkSmartPointer<vtkPlaneSource>::New();
	aPlane->SetXResolution(amountOfBins);
	aPlane->SetYResolution(m_ColForData);

	double x = m_rowSize;
	double y = (m_colSize * currentColumn) + offset;
	aPlane->SetOrigin(0, y, 0.0);
	aPlane->SetPoint1(x, y, 0.0);               //width
	aPlane->SetPoint2(0, y + m_colSize, 0.0);  // height
	aPlane->Update();

	vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorData->SetName("colors");
	colorData->SetNumberOfComponents(3);
	colorRow(colorData, currDataInd, amountOfBins);
	aPlane->GetOutput()->GetCellData()->SetScalars(colorData);

	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(aPlane->GetOutputPort());
	mapper->SetScalarModeToUseCellData();
	mapper->Update();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);

	if(!m_useDarkerLut)
	{ //the edges of the cells are drawn
		actor->GetProperty()->EdgeVisibilityOn();
		double col[3];
		iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY,col);
		actor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
	}else
	{ //not showing the edges of the cells
		actor->GetProperty()->EdgeVisibilityOff();
	}

	m_renderer->AddActor(actor);

	//store row and for each row the index which dataset it is showing
	m_originalPlaneActors->push_back(actor);
	m_rowDataIndexPair->insert({ actor, currDataInd });

	//add name of dataset/row
	double pos[3] = { -m_rowSize*0.05, y + (m_colSize*0.5), 0.0 };
	addDatasetName(currDataInd, pos);

	return aPlane;
}

std::vector<vtkSmartPointer<vtkPlaneSource>>* iACompHistogramTable::drawZoomedRow(
	/*int currDataInd,*/ int currentColumn, int amountOfBins, bin::BinType* currentData, double offsetHeight, std::vector<vtkIdType>* cellIdsOriginalPlane)
{
	std::vector<vtkSmartPointer<vtkPlaneSource>>* zoomedPlanes = new std::vector<vtkSmartPointer<vtkPlaneSource>>();

	double widthOriginal = m_rowSize + m_rowSize * 0.7;
	double widthRange = widthOriginal / currentData->size();
	double offsetWidth = m_rowSize * 0.05;
	
	double startX = -m_rowSize * 0.35;
	double endX = startX + widthRange;
	double startY = (m_colSize * currentColumn) + offsetHeight;
	double endY = startY + (1.5*m_colSize);

	for(int i = 0; i < ((int)cellIdsOriginalPlane->size()); i++)
	{
		vtkSmartPointer<vtkPlaneSource> plane = drawZoomedPlanes(amountOfBins, startX, startY, endX, endY, i, currentData);

		zoomedPlanes->push_back(plane);

		if ( i < (((int)cellIdsOriginalPlane->size())-1))
		{
			vtkIdType lastCellId = cellIdsOriginalPlane->at(i);
			vtkIdType currCellId = cellIdsOriginalPlane->at(i + 1);

			if ((lastCellId + 1) != currCellId)
			{
				startX = endX + offsetWidth;
			}
			else
			{
				startX = endX;
			}
		}
		
		endX = startX + widthRange;
	}

	return zoomedPlanes;
}

vtkSmartPointer<vtkPlaneSource> iACompHistogramTable::drawZoomedPlanes(int bins, double startX, double startY, double endX, double endY, int currBinIndex, bin::BinType* currentData)
{
	vtkSmartPointer<vtkPlaneSource> aPlane = vtkSmartPointer<vtkPlaneSource>::New();
	aPlane->SetXResolution(bins);
	aPlane->SetYResolution(m_ColForData);

	aPlane->SetOrigin(startX, startY, 0.0);
	aPlane->SetPoint1(endX, startY, 0.0); //width
	aPlane->SetPoint2(startX, endY, 0.0); // height
	aPlane->Update(); 

	vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorData->SetName("colors");
	colorData->SetNumberOfComponents(3);

	colorRowForZoom(colorData, currBinIndex, currentData, bins);
	aPlane->GetOutput()->GetCellData()->SetScalars(colorData);

	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(aPlane->GetOutputPort());
	mapper->SetScalarModeToUseCellData();
	mapper->Update();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	actor->GetProperty()->EdgeVisibilityOn();
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY,col);
	actor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
	actor->GetProperty()->SetLineWidth(1);

	vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
	actor->GetProperty()->SetColor(nc->GetColor3d("DarkGray").GetData());
	m_renderer->AddActor2D(actor);

	m_zoomedPlaneActors->push_back(actor);

	return aPlane;
}

void iACompHistogramTable::redrawZoomedRow(int selectedBinNumber)
{	
	int rowId = m_zoomedRowData->size()-1;
	int cellId = 0;
	for (int zoomedRowDataInd = 0; zoomedRowDataInd < ((int)m_zoomedPlaneActors->size()); zoomedRowDataInd++)
	{ //for all zoomed planes

		vtkSmartPointer<vtkActor> currAct = m_zoomedPlaneActors->at(zoomedRowDataInd);
		vtkSmartPointer<vtkAlgorithm> algorithm = currAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
		
		vtkSmartPointer<vtkPlaneSource> plane = vtkPlaneSource::SafeDownCast(algorithm);
		plane->SetXResolution(selectedBinNumber);
		plane->Update();

		vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
		colorData->SetName("colors");
		colorData->SetNumberOfComponents(3);

		/*LOG(lvlDebug,"rowId = " + QString::number(rowId));
		LOG(lvlDebug,"cellId = " + QString::number(cellId));
		LOG(lvlDebug,"m_zoomedRowData->size() = " + QString::number(m_zoomedRowData->size()));
		LOG(lvlDebug,"m_zoomedRowData->size() - 1 = " + QString::number(m_zoomedRowData->size() - 1));*/

		colorRowForZoom(colorData, cellId, m_zoomedRowData->at(rowId), selectedBinNumber);
		plane->GetOutput()->GetCellData()->SetScalars(colorData);
		currAct->Modified();

		//get the correct data bins of zoomedRowData
		if (cellId >= (((int)m_zoomedRowData->at(rowId)->size()) - 1) )
		{ 
			rowId--;
			cellId = 0;
		}
		else
		{
			cellId++;
		}
	}

	renderWidget();
}

void iACompHistogramTable::drawPointRepresentation()
{
	std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>* >::iterator it;
	int zoomedRowDataInd = m_zoomedRowData->size() - 1;
	
	//find max-min val of each dataset and same bin to enable comparison of the same bins in different datasets
	std::map<int, std::vector<double>*> minMaxPerBin = std::map<int, std::vector<double>*>();
	for (int datasetInd = 0; datasetInd < ((int)m_zoomedRowData->size()); datasetInd++)
	{
		for (int binId = 0; binId < ((int)m_zoomedRowData->at(datasetInd)->size()); binId++)
		{
			minMaxPerBin.insert({binId, new std::vector<double>()});

			for (int valId = 0; valId < ((int)m_zoomedRowData->at(datasetInd)->at(binId).size()); valId++)
			{
				auto binData = m_zoomedRowData->at(datasetInd)->at(binId);
				auto minMax = std::minmax_element(binData.begin(), binData.end());
				double currMin = *minMax.first;
				double currMax = *minMax.second;

				std::vector<double>* minMaxVec = minMaxPerBin.at(binId);
				if(minMaxVec->size() == 0)
				{ // if empty --> initialize
					minMaxVec->push_back(currMin);
					minMaxVec->push_back(currMax);
				}else
				{
					if (minMaxVec->at(0) > currMin)
					{
						minMaxVec->at(0) = currMin;
					}

					if(minMaxVec->at(1) < currMax)
					{
						minMaxVec->at(1) = currMax;
					}
				}
			}
		}
	}

	//draw point presentation
	for (int counter = 0; counter < m_amountDatasets; counter++)
	{
		int indData = m_orderOfIndicesDatasets->at(counter);
	
		it = originalPlaneZoomedPlanePair->find(m_originalPlaneActors->at(counter));

		if(it != originalPlaneZoomedPlanePair->end())
		{  //row
			vtkSmartPointer<vtkActor> originalRowAct = it->first;
			std::vector<vtkSmartPointer<vtkActor>>* zoomedRowActs = it->second;

			vtkSmartPointer<vtkAlgorithm> algorithm1 = originalRowAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
			vtkSmartPointer<vtkPlaneSource> originalPlane = vtkPlaneSource::SafeDownCast(algorithm1);
			
			for(int zoomedRowInd = 0; zoomedRowInd < ((int)zoomedRowActs->size()); zoomedRowInd++)
			{
				vtkSmartPointer<vtkActor> zoomedRowAct = zoomedRowActs->at(zoomedRowInd);
				vtkSmartPointer<vtkAlgorithm> algorithm = zoomedRowAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
				vtkSmartPointer<vtkPlaneSource> zoomedPlane = vtkPlaneSource::SafeDownCast(algorithm);

				double xmin = zoomedPlane->GetOrigin()[0];
				double xmax = zoomedPlane->GetPoint1()[0];
				double ymin = zoomedPlane->GetOrigin()[1];
				double ymax = zoomedPlane->GetPoint2()[1];

				auto iter = m_pickedCellsforPickedRow->find(indData);
				if (iter == m_pickedCellsforPickedRow->end()) { continue; }

				//set plane
				vtkSmartPointer<vtkPlaneSource> pointPlane = vtkSmartPointer<vtkPlaneSource>::New();
				pointPlane->SetXResolution(1);
				pointPlane->SetYResolution(1);

				pointPlane->SetOrigin(xmin, ymin, 0.0);
				pointPlane->SetPoint1(xmax, ymin, 0.0);
				pointPlane->SetPoint2(xmin, ymax, 0.0);
				pointPlane->Update();

				vtkSmartPointer<vtkPolyDataMapper> planeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
				planeMapper->SetInputConnection(pointPlane->GetOutputPort());
				planeMapper->SetScalarModeToUseCellData();
				planeMapper->Update();

				vtkSmartPointer<vtkActor> planeActor = vtkSmartPointer<vtkActor>::New();
				planeActor->SetMapper(planeMapper);
				planeActor->GetProperty()->SetEdgeVisibility(true);
				double col[3];
				iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK,col);
				planeActor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
				m_pointRepresentationActors->push_back(planeActor);
				m_renderer->AddActor(planeActor);

				//set middle line
				double startP[3] = { xmin, ymin + ((ymax - ymin) / 2.0), 0.0 };
				double endP[3] = { xmax, ymin + ((ymax - ymin) / 2.0), 0.0 };
				double col1[3];
				iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY,col1);
				vtkSmartPointer<vtkActor> lineActor = drawLine(startP, endP, col1, 2);
				lineActor->GetProperty()->SetOpacity(0.1);
				lineActor->Modified();
				m_pointRepresentationActors->push_back(lineActor);

				//draw indiviudal data points
				double radius = (m_colSize*0.5)*0.25;
				double lineWidth = 1;
				double circleColor[3];
				iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, circleColor);
				double lineColor[3];
				iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, lineColor);

				std::vector<double> zoomedData = m_zoomedRowData->at(zoomedRowDataInd)->at(zoomedRowInd);


				double newY = ymin + ((ymax - ymin) / 2.0);
				//(newXMin + radius) --> so that min & max points do not lie on border
				vtkSmartPointer<vtkPoints> points = calculatePointPosition(zoomedData, (xmin + radius), (xmax - radius), newY, *minMaxPerBin.at(zoomedRowInd));
				if (points == nullptr)
				{
					continue;
				}

				vtkSmartPointer<vtkActor> pointActor = drawPoints(points, circleColor, radius, lineColor, lineWidth);
				m_pointRepresentationActors->push_back(pointActor);
			}
			zoomedRowDataInd--;
		}	
	}

	renderWidget();
}

void iACompHistogramTable::removePointRepresentation()
{
	for (int i = 0; i < ((int)m_pointRepresentationActors->size()); i++)
	{
		m_renderer->RemoveActor(m_pointRepresentationActors->at(i));
	}

	m_pointRepresentationActors->clear();
}

vtkSmartPointer<vtkPoints> iACompHistogramTable::calculatePointPosition(
	std::vector<double> dataPoints, double newMinX, double newMaxX, double y, std::vector<double> /*currMinMax*/)
{

	/*LOG(lvlDebug,"dataPoints.size() = " + QString::number(dataPoints.size()));

	for (int i = 0; i < dataPoints.size(); i++)
	{
		LOG(lvlDebug,"Point " + QString::number(i) + ": " + QString::number(dataPoints.at(i)));
	}*/

	if(dataPoints.size() == 0)
	{ //when the list is empty
		return nullptr;
	}else if(dataPoints.size() == 1)
	{ //min and max are the same
		vtkSmartPointer<vtkPoints> result = vtkSmartPointer<vtkPoints>::New();
		result->InsertNextPoint(newMinX + ((newMaxX-newMinX)*0.5), y, 0.0);
		return result;
	}

	vtkSmartPointer<vtkPoints> result = vtkSmartPointer<vtkPoints>::New();
	auto minMax = std::minmax_element(dataPoints.begin(), dataPoints.end());
	double min = *minMax.first;
	double max = *minMax.second;

	//use when point representation should be comparable
	/*double min = currMinMax.at(0);
	double max = currMinMax.at(1);*/

	for (int i = 0; i < ((int)dataPoints.size()); i++)
	{
		if (min == max)
		{
			result->InsertNextPoint(newMaxX, y, 0.0);
		}
		else
		{
			double x = iACompVisOptions::histogramNormalization(dataPoints.at(i), newMinX, newMaxX, min, max);
			result->InsertNextPoint(x, y, 0.0);
		}
	}

	return result;
}

void iACompHistogramTable::drawLineBetweenRowAndZoomedRow(std::vector<vtkSmartPointer<vtkPlaneSource>>* zoomedRowPlanes, vtkSmartPointer<vtkPlaneSource> originalRowPlane, std::vector<vtkIdType>* cellIdsOriginalPlane)
{
	int amountOfCells = originalRowPlane->GetXResolution();
	double xMinO = originalRowPlane->GetOrigin()[0];
	double yMinO = originalRowPlane->GetOrigin()[1];
	double xMaxO = originalRowPlane->GetPoint1()[0];
	double widthO = xMaxO - xMinO;
	double binLengthO = widthO / ((double)amountOfCells);

	for (int i = 0; i < ((int)zoomedRowPlanes->size()); i++) 
	{
		vtkSmartPointer<vtkPlaneSource> zoomedRowPlane = zoomedRowPlanes->at(i);
		double xMinZ = zoomedRowPlane->GetOrigin()[0];
		double xMaxZ = zoomedRowPlane->GetPoint1()[0];
		double yMinZ = zoomedRowPlane->GetOrigin()[1];
		double yMaxZ = zoomedRowPlane->GetPoint2()[1];

		double currCellId = cellIdsOriginalPlane->at(i);

		double col[3];
		iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN,col);

		//left line
		double p0[3];
		p0[0] = xMinO + (binLengthO * currCellId);
		p0[1] = yMinO;
		p0[2] = 0.0;

		double p1[3];
		p1[0] = xMinZ;
		p1[1] = yMaxZ;
		p1[2] = 0.0;

		drawLine(p0, p1, col, iACompVisOptions::LINE_WIDTH);

		//right line
		double p2[3];
		p2[0] = p0[0] + binLengthO;
		p2[1] = p0[1];
		p2[2] = p0[2];

		double p3[3];
		p3[0] = xMaxZ;
		p3[1] = p1[1];
		p3[2] = p1[2];

		drawLine(p2, p3, col, iACompVisOptions::LINE_WIDTH);

		//border zoomed row
		double leftDown[3] = { p1[0], yMinZ, 0.0 };
		double rightDown[3] = { p3[0], yMinZ, 0.0 };
		double rightUp[3] = { p3[0], p3[1], p3[2] };
		double leftUp[3] = { p1[0], p1[1], p1[2] };
		
		if(i == 0)
		{
			//left line
			vtkSmartPointer<vtkPoints> pointsLeft = vtkSmartPointer<vtkPoints>::New();
			pointsLeft->InsertNextPoint(leftUp);
			pointsLeft->InsertNextPoint(leftDown);
			drawPolyLine(pointsLeft, col, iACompVisOptions::LINE_WIDTH);

			if(cellIdsOriginalPlane->size() > 1)
			{
				double nextCellId = cellIdsOriginalPlane->at(i + 1);
				if ((currCellId + 1) != nextCellId)
				{
					//right line
					vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
					points->InsertNextPoint(rightUp);
					points->InsertNextPoint(rightDown);
					drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);
				}
			}else
			{   //right line
				vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
				points->InsertNextPoint(rightUp);
				points->InsertNextPoint(rightDown);
				drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);
			}
			
		}
		else if (i == (((int)cellIdsOriginalPlane->size()) - 1))
		{	
			//right line
			vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
			points->InsertNextPoint(rightUp);
			points->InsertNextPoint(rightDown);
			drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);

			double lastCellId = cellIdsOriginalPlane->at(i - 1);
			if ((currCellId - 1.0) != lastCellId)
			{
				//left line
				vtkSmartPointer<vtkPoints> points1 = vtkSmartPointer<vtkPoints>::New();
				points1->InsertNextPoint(leftUp);
				points1->InsertNextPoint(leftDown);
				drawPolyLine(points1, col, iACompVisOptions::LINE_WIDTH);
			}
		}
		else if (i < ((int)cellIdsOriginalPlane->size()) - 1)
		{
			//double currCellId = cellIdsOriginalPlane->at(i);
			double nextCellId = cellIdsOriginalPlane->at(i + 1);
			double lastCellId = cellIdsOriginalPlane->at(i - 1);

			if ( (currCellId+1) != nextCellId)
			{
				//right line
				vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
				points->InsertNextPoint(rightUp);
				points->InsertNextPoint(rightDown);
				drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);
			}
			
			if ((currCellId - 1) != lastCellId)
			{
				//left line
				vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
				points->InsertNextPoint(leftUp);
				points->InsertNextPoint(leftDown);
				drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);
			}
		}
		
		vtkSmartPointer<vtkPoints> pointsAbove = vtkSmartPointer<vtkPoints>::New();
		pointsAbove->InsertNextPoint(leftUp);
		pointsAbove->InsertNextPoint(rightUp);
		drawPolyLine(pointsAbove, col, iACompVisOptions::LINE_WIDTH);

		vtkSmartPointer<vtkPoints> pointsDown = vtkSmartPointer<vtkPoints>::New();
		pointsDown->InsertNextPoint(leftDown);
		pointsDown->InsertNextPoint(rightDown);
		drawPolyLine(pointsDown, col, iACompVisOptions::LINE_WIDTH);
	}
}

vtkSmartPointer<vtkActor> iACompHistogramTable::drawLine(double* startPoint, double* endPoint, double lineColor[3], double lineWidth)
{
	vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
	lineSource->SetPoint1(startPoint);
	lineSource->SetPoint2(endPoint);
	lineSource->Update();

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(lineSource->GetOutputPort());
	vtkSmartPointer<vtkActor> lineActor = vtkSmartPointer<vtkActor>::New();
	lineActor->SetMapper(mapper);
	
	lineActor->GetProperty()->SetColor(lineColor[0], lineColor[1], lineColor[2]);
	lineActor->GetProperty()->SetLineWidth(lineWidth);
	lineActor->GetProperty()->RenderLinesAsTubesOn();
	m_renderer->AddActor(lineActor);

	return lineActor;
}

vtkSmartPointer<vtkActor> iACompHistogramTable::drawPolyLine(vtkSmartPointer<vtkPoints> points, double lineColor[3], double lineWidth)
{
	vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
	polyLine->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
	for (unsigned int i = 0; i < points->GetNumberOfPoints(); i++)
	{
		polyLine->GetPointIds()->SetId(i, i);
	}

	vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
	cells->InsertNextCell(polyLine);

	vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
	polyData->SetPoints(points);
	polyData->SetLines(cells);

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData(polyData);

	vtkSmartPointer<vtkActor> polyLineActor = vtkSmartPointer<vtkActor>::New();
	polyLineActor->SetMapper(mapper);
	polyLineActor->GetProperty()->SetColor(lineColor[0], lineColor[1], lineColor[2]);
	polyLineActor->GetProperty()->SetLineWidth(lineWidth);

	m_renderer->AddActor(polyLineActor);

	return polyLineActor;
}

vtkSmartPointer<vtkActor> iACompHistogramTable::drawPoints(vtkSmartPointer<vtkPoints> points, double color[3], double radius, double lineColor[3], double lineWidth)
{
	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->SetPoints(points);

	vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
	sphereSource->SetRadius(radius);
	/*vtkSmartPointer<vtkRegularPolygonSource> polygonSource = vtkSmartPointer<vtkRegularPolygonSource>::New();
	polygonSource->SetNumberOfSides(50);
	polygonSource->SetRadius(radius);
	polygonSource->Update();*/

	vtkSmartPointer<vtkGlyph2D> glyph2D = vtkSmartPointer<vtkGlyph2D>::New();
	//glyph2D->SetSourceConnection(polygonSource->GetOutputPort());
	glyph2D->SetSourceConnection(sphereSource->GetOutputPort());
	glyph2D->SetInputData(polydata);
	glyph2D->Update();

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(glyph2D->GetOutputPort());
	mapper->Update();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	actor->GetProperty()->SetColor(color[0], color[1], color[2]);
	//actor->GetProperty()->SetEdgeVisibility(true);
	actor->GetProperty()->SetLineWidth(lineWidth);
	actor->GetProperty()->SetEdgeColor(lineColor[0], lineColor[1], lineColor[2]);

	m_renderer->AddActor(actor);

	return actor;
}

void iACompHistogramTable::renderWidget()
{
	m_qvtkWidget->renderWindow()->GetInteractor()->Render();
}

/******************************************  Coloring (LookupTable)  **********************************/
void iACompHistogramTable::makeLUTFromCTF()
{
	calculateBinLength();

	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToRGB();

	// red to white
	/*QColor red = QColor(164, 0, 0, 255);
	QColor white = QColor(255, 255, 255, 255);

	ctf->AddRGBPoint(1.0, red.redF(), red.greenF(), red.blueF());
	ctf->AddRGBPoint(0.9, red.redF(), red.greenF(), red.blueF());
	ctf->AddRGBPoint(0.0, white.redF(), white.greenF(), white.blueF());*/

	//diverging coloring from red to yellow to blue
	/*QColor c1 = QColor(165, 0, 38);
	QColor c2 = QColor(215, 48, 39);
	QColor c3 = QColor(244, 109, 67);
	QColor c4 = QColor(253, 174, 97);
	QColor c5 = QColor(254, 224, 144);
	QColor c6 = QColor(224, 243, 248);
	QColor c7 = QColor(171, 217, 233);
	QColor c8 = QColor(116, 173, 209);
	QColor c9 = QColor(69, 117, 180);
	QColor c10 = QColor(49, 54, 149);*/

	//sequential pink to green/bluish in pastel
	/*QColor c1 = QColor(47, 117, 131);
	QColor c2 = QColor(55, 136, 153);
	QColor c3 = QColor(109, 177, 190);
	QColor c4 = QColor(144, 158, 195);
	QColor c5 = QColor(162, 148, 201);
	QColor c6 = QColor(188, 178, 215);
	QColor c7 = QColor(208, 185, 205);
	QColor c8 = QColor(203, 149, 192);
	QColor c9 = QColor(234, 185, 214);
	QColor c10 = QColor(255, 255, 255);*/

	QColor c1 = QColor(103, 21, 45); 
	QColor c2 = QColor(128, 0, 38);
	QColor c3 = QColor(189, 0, 38);
	QColor c4 = QColor(227, 26, 28);
	QColor c5 = QColor(252, 78, 42);
	QColor c6 = QColor(253, 141, 60);
	QColor c7 = QColor(254, 178, 76);
	QColor c8 = QColor(254, 217, 118);
	QColor c9 = QColor(255, 237, 160);
	QColor c10 = QColor(255, 255, 204);

	ctf->AddRGBPoint(1.0, c1.redF(), c1.greenF(), c1.blueF());
	ctf->AddRGBPoint(0.9, c1.redF(), c1.greenF(), c1.blueF());
	ctf->AddRGBPoint(0.8, c2.redF(), c2.greenF(), c2.blueF());
	ctf->AddRGBPoint(0.7, c3.redF(), c3.greenF(), c3.blueF());
	ctf->AddRGBPoint(0.6, c4.redF(), c4.greenF(), c4.blueF());
	ctf->AddRGBPoint(0.5, c5.redF(), c5.greenF(), c5.blueF());
	ctf->AddRGBPoint(0.4, c6.redF(), c6.greenF(), c6.blueF());
	ctf->AddRGBPoint(0.3, c7.redF(), c7.greenF(), c7.blueF());
	ctf->AddRGBPoint(0.2, c8.redF(), c8.greenF(), c8.blueF());
	ctf->AddRGBPoint(0.1, c9.redF(), c9.greenF(), c9.blueF());
	ctf->AddRGBPoint(0.0, c10.redF(), c10.greenF(), c10.blueF());

	m_lut->SetNumberOfTableValues(m_tableSize);
	m_lut->Build();

	double min = 0;
	double max = 0;
	int startVal = 0;

	for (size_t i = 0; i < ((size_t)m_tableSize); i++)
	{
		double* rgb;
		rgb = ctf->GetColor(static_cast<double>(i) / (double)m_tableSize);
		m_lut->SetTableValue(i, rgb);

		//make format of annotations
		double low = round_up(startVal + (i * m_BinRangeLength), 2);
		double high = round_up(startVal + ((i + 1) * m_BinRangeLength), 2);
		
		std::string sLow = std::to_string(low);
		std::string sHigh = std::to_string(high);

		std::string lowerString = initializeLegendLabels(sLow);
		std::string upperString = initializeLegendLabels(sHigh);

		//position description in the middle of each color bar in the scalarBar legend
		m_lut->SetAnnotation(low + ((high-low)*0.5), lowerString + " - " + upperString);
		
		//store min and max value of the dataset
		if (i == 0) 
		{
			min = low;
		}else if( i == ((size_t)(m_tableSize-1)))
		{
			max = high;
		}
	}

	m_lut->SetTableRange(min, max);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	m_lut->SetBelowRangeColor(col[0], col[1], col[2], 1);
	m_lut->UseBelowRangeColorOn();
}

void iACompHistogramTable::makeLUTDarker()
{
	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToRGB();

	QColor c1 = QColor(51, 10, 23);
	QColor c2 = QColor(64, 0, 19);
	QColor c3 = QColor(64, 0, 19);
	QColor c4 = QColor(113, 13, 14);
	QColor c5 = QColor(126, 39, 21);
	QColor c6 = QColor(126, 70, 30);
	QColor c7 = QColor(127, 89, 38);
	QColor c8 = QColor(127, 108, 59);
	QColor c9 = QColor(127, 118, 80);
	QColor c10 = QColor(127, 127, 102);

	ctf->AddRGBPoint(1.0, c1.redF(), c1.greenF(), c1.blueF());
	ctf->AddRGBPoint(0.9, c1.redF(), c1.greenF(), c1.blueF());
	ctf->AddRGBPoint(0.8, c2.redF(), c2.greenF(), c2.blueF());
	ctf->AddRGBPoint(0.7, c3.redF(), c3.greenF(), c3.blueF());
	ctf->AddRGBPoint(0.6, c4.redF(), c4.greenF(), c4.blueF());
	ctf->AddRGBPoint(0.5, c5.redF(), c5.greenF(), c5.blueF());
	ctf->AddRGBPoint(0.4, c6.redF(), c6.greenF(), c6.blueF());
	ctf->AddRGBPoint(0.3, c7.redF(), c7.greenF(), c7.blueF());
	ctf->AddRGBPoint(0.2, c8.redF(), c8.greenF(), c8.blueF());
	ctf->AddRGBPoint(0.1, c9.redF(), c9.greenF(), c9.blueF());
	ctf->AddRGBPoint(0.0, c10.redF(), c10.greenF(), c10.blueF());

	m_lutDarker->SetNumberOfTableValues(m_tableSize);
	m_lutDarker->Build();

	double min = 0;
	double max = 0;
	int startVal = 0;

	for (size_t i = 0; i < ((size_t)m_tableSize); i++)
	{
		double* rgb;
		rgb = ctf->GetColor(static_cast<double>(i) / (double)m_tableSize);
		m_lutDarker->SetTableValue(i, rgb);

		//make format of annotations
		double low = round_up(startVal + (i * m_BinRangeLength), 2);
		double high = round_up(startVal + ((i + 1) * m_BinRangeLength), 2);

		std::string sLow = std::to_string(low);
		std::string sHigh = std::to_string(high);

		std::string lowerString = initializeLegendLabels(sLow);
		std::string upperString = initializeLegendLabels(sHigh);

		//position description in the middle of each color bar in the scalarBar legend
		m_lutDarker->SetAnnotation(low + ((high - low)*0.5), lowerString + " - " + upperString);

		//store min and max value of the dataset
		if (i == 0)
		{
			min = low;
		}
		else if (i == ((size_t)(m_tableSize - 1)))
		{
			max = high;
		}
	}

	m_lutDarker->SetTableRange(min, max);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY,col);
	m_lutDarker->SetBelowRangeColor(col[0], col[1], col[2], 1);
	m_lutDarker->UseBelowRangeColorOn();

}

void iACompHistogramTable::colorRow(vtkUnsignedCharArray* colors, int currDataset, int numberOfBins)
{
	QList<bin::BinType*>* binData = m_histData->calculateBins(numberOfBins);
	calculateBinLength();

	colorBinsOfRow(colors, binData->at(currDataset), binData->at(currDataset)->size());
}

void iACompHistogramTable::colorRowForZoom(vtkUnsignedCharArray* colors, int currBin, bin::BinType* curData, int amountOfBins)
{
	bin::BinType* binData = m_histData->calculateBins(curData, currBin, amountOfBins);

	if (binData == nullptr)
	{
		for (int b = 0; b < amountOfBins; b++)
		{
			double rgb[3];
			m_lut->GetColor(-1, rgb);
			unsigned char ucrgb[3];
			iACompVisOptions::getColorArray(rgb, ucrgb);
			colors->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
		}
		return;
	}

	colorBinsOfRow(colors, binData, binData->size());
}

void iACompHistogramTable::colorBinsOfRow(vtkUnsignedCharArray* colors, bin::BinType* binData, int amountOfBins)
{
	for (int b = 0; b < amountOfBins; b++)
	{  //for each selected bin a specific amount of bins is drawn according to this data

		double rgb[3];
		int amountVals = binData->at(b).size();
		
		if(m_useDarkerLut)
		{
			m_lutDarker->GetColor(amountVals, rgb);
		}else
		{
			m_lut->GetColor(amountVals, rgb);
		}
		
		unsigned char ucrgb[3];
		iACompVisOptions::getColorArray(rgb,ucrgb);
		colors->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
	}
}

void iACompHistogramTable::calculateBinLength()
{
	int maxAmountInAllBins = m_histData->getMaxAmountInAllBins();
	m_BinRangeLength = ((double)maxAmountInAllBins) / ((double)m_tableSize);
}

/******************************************  Initialization ******************************************/
void iACompHistogramTable::initializeHistogramTable()
{
	//setup color table
	makeLUTFromCTF();
	makeLUTDarker();

	//initialize legend
	initializeLegend();

	//initialize interaction
	initializeInteraction();

	//initialize row of dataset index
	initializeOrderOfIndices();

	//draw histogramTable
	drawHistogramTable(m_bins);

	//init camera
	vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
	camera->SetPosition(m_rowSize*0.5, (m_colSize*m_amountDatasets)*0.5, 1);
	camera->SetFocalPoint( m_rowSize*0.5, (m_colSize*m_amountDatasets)*0.5, 0);
	m_renderer->SetActiveCamera(camera);

	renderWidget();
}

void iACompHistogramTable::initializeOrderOfIndices()
{
	int dataIndex = m_amountDatasets - 1;
	for(int position = 0; position < m_amountDatasets; position++)
	{
		m_orderOfIndicesDatasets->at(position) = dataIndex;
		m_originalOrderOfIndicesDatasets->at(position) = dataIndex;

		dataIndex--;
	}
}

void iACompHistogramTable::initializeLegend()
{
	vtkSmartPointer<vtkScalarBarActor> scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
	scalarBar->SetLookupTable(m_lut);
	scalarBar->SetHeight(0.85);
	scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	scalarBar->GetPositionCoordinate()->SetValue(0.001, 0.1, 0.0);
	scalarBar->SetWidth(0.5);
	scalarBar->SetUnconstrainedFontSize(1);

	scalarBar->SetTitle("                   Amount of Objects");
	scalarBar->SetNumberOfLabels(0);
	scalarBar->SetTextPositionToPrecedeScalarBar();

	//title properties
	scalarBar->GetTitleTextProperty()->BoldOn();
	scalarBar->GetTitleTextProperty()->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TITLE, col);
	scalarBar->GetTitleTextProperty()->SetColor(col);
	scalarBar->GetTitleTextProperty()->SetVerticalJustificationToTop();
	scalarBar->SetVerticalTitleSeparation(7);
	scalarBar->GetTitleTextProperty()->Modified();

	//text properties
	vtkSmartPointer<vtkTextProperty> propL = vtkSmartPointer<vtkTextProperty>::New();
	propL->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	double col1[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TEXT,col1);
	propL->SetColor(col1);
	propL->Modified();
	scalarBar->SetAnnotationTextProperty(propL);

	// Setup render window, renderer, and interactor
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
	renderer->SetViewport(0.8, 0, 1, 1);
	double col2[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY,col2);
	renderer->SetBackground(col2);
	renderer->AddActor2D(scalarBar);

	m_qvtkWidget->renderWindow()->AddRenderer(renderer);
}

std::string iACompHistogramTable::initializeLegendLabels(std::string input)
{
	std::string result;
	std::string helper = input;
	std::string newLow = input.erase(input.find('.'), std::string::npos);

	if (newLow.size() > 1)
	{  //more than one charachater before the dot
		result = newLow;
	}
	else
	{  //only one  character before the dot
		result = helper.erase(helper.find('.') + 2, std::string::npos);
	}

	return result;
}

void iACompHistogramTable::addDatasetName(int currDataset, double* position)
{
	QStringList* filenames = m_dataStorage->getDatasetNames();
	std::string name = filenames->at(currDataset).toLocal8Bit().constData();
	name.erase(0, name.find_last_of("/\\") + 1);

	vtkSmartPointer<vtkTextActor> legend = vtkSmartPointer<vtkTextActor>::New();
	legend->SetTextScaleModeToNone();
	legend->SetInput(name.c_str());
	legend->GetPositionCoordinate()->SetCoordinateSystemToWorld();
	legend->GetPositionCoordinate()->SetValue(position[0], position[1], position[2]);

	vtkSmartPointer<vtkTextProperty> legendProperty = legend->GetTextProperty();
	legendProperty->BoldOn();
	legendProperty->ItalicOff();
	legendProperty->ShadowOn();
	legendProperty->SetFontFamilyToArial();
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TITLE, col);
	legendProperty->SetColor(col);
	legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	legendProperty->SetVerticalJustificationToCentered();
	legendProperty->SetJustification(VTK_TEXT_RIGHT);
	legendProperty->Modified();

	m_renderer->AddActor(legend);
}

void iACompHistogramTable::initializeInteraction()
{
	vtkSmartPointer<iACompHistogramTableInteractorStyle> style =
		vtkSmartPointer<iACompHistogramTableInteractorStyle>::New();
	style->setIACompHistogramTable(this);
	style->SetDefaultRenderer(m_renderer);
	style->setIACompVisMain(m_main);

	m_qvtkWidget->interactor()->SetInteractorStyle(style);
}

/******************************************  Getter & Setter ******************************************/
int iACompHistogramTable::getBins()
{
	return m_bins;
}

void iACompHistogramTable::setBins(int bins)
{
	m_bins = bins;
}

int iACompHistogramTable::getMinBins()
{
	return minBins;
}

int iACompHistogramTable::getMaxBins()
{
	return maxBins;
}

int iACompHistogramTable::getBinsZoomed()
{
	return m_binsZoomed;
}

void iACompHistogramTable::setBinsZoomed(int bins)
{
	m_binsZoomed = bins;
}

vtkSmartPointer<vtkRenderer> iACompHistogramTable::getRenderer()
{
	return m_renderer;
}

std::vector<vtkSmartPointer<vtkActor>>* iACompHistogramTable::getOriginalRowActors()
{
	return m_originalPlaneActors;
}

double iACompHistogramTable::round_up(double value, int decimal_places)
{
	const double multiplier = std::pow(10.0, decimal_places);
	return std::ceil(value * multiplier) / multiplier;
}

std::vector<int>* iACompHistogramTable::getIndexOfPickedRows()
{
	return m_indexOfPickedRow;
}

std::vector<int>* iACompHistogramTable::getAmountObjectsEveryDataset()
{
	return csvFileData::getAmountObjectsEveryDataset(m_dataStorage->getData());
}

vtkSmartPointer<vtkActor> iACompHistogramTable::getHighlightingRowActor()
{
	return m_highlightRowActor;
}

/******************************************  Data Caculation  **********************************************/
void iACompHistogramTable::calculateHistogramTable()
{
	m_histData = new iACompHistogramTableData(m_mds, m_dataStorage);
}

/******************************************  Interaction  **********************************************/
std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> iACompHistogramTable::getSelectedData(Pick::PickedMap* map)
{
	//stores object attributes
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjects = new QList<std::vector<csvDataType::ArrayType*>*>();
	//stores MDS values
	QList<bin::BinType*>* thisZoomedRowData = new QList<bin::BinType*>();

	//get for all datasets each bin with its objects attributes (label,...)
	QList<std::vector<csvDataType::ArrayType*>*>* objectsPerBin = m_histData->getObjectsPerBin();
	//get for all dataset each bin with its MDS values
	QList<bin::BinType*>* binData = m_histData->getBinData();

	m_indexOfPickedRow = new std::vector<int>();
	m_pickedCellsforPickedRow = new std::map<int, std::vector<vtkIdType>*>();

	for (int rowId = m_amountDatasets-1; rowId >= 0; rowId--)
	{
		vtkSmartPointer<vtkActor> currAct = m_originalPlaneActors->at(rowId);

		//set dataIndex
		std::map<vtkSmartPointer<vtkActor>, int>::iterator it = m_rowDataIndexPair->find(currAct);
		if (it == m_rowDataIndexPair->end()) continue;
		int dataIndex = it->second;

		//for every row --> for every actor
		if (map->find(currAct) != map->end())
		{
			std::vector<vtkIdType>* pickedCells = map->find(currAct)->second;
			std::sort(pickedCells->begin(), pickedCells->end(), std::less<long long>());

			//store Ids
			std::vector<csvDataType::ArrayType*>* currRowIds = objectsPerBin->at(dataIndex);
			std::vector<csvDataType::ArrayType*>* newRowIds = new std::vector<csvDataType::ArrayType*>();

			//store MDS values
			bin::BinType* currRowMDS = binData->at(dataIndex);
			bin::BinType* newRowMDS = new bin::BinType();

			//look for the selected cells in the current row
			for (int i = 0; i < ((int)pickedCells->size()); i++)
			{
				int currBin = pickedCells->at(i);
				newRowIds->push_back(currRowIds->at(currBin));
				newRowMDS->push_back(currRowMDS->at(currBin));
			}

			selectedObjects->append(newRowIds);
			thisZoomedRowData->append(newRowMDS);

			m_indexOfPickedRow->push_back(dataIndex);
			m_pickedCellsforPickedRow->insert({ dataIndex, pickedCells });
		}
	}

	//DEBUG
	/*LOG(lvlDebug,"DEBUGGING");
	for (int i = 0; i < thisZoomedRowData->size(); i++)
	{
		bin::debugBinType(thisZoomedRowData->at(i));
	}*/

	/*LOG(lvlDebug,"DEBUGGING");
	for (int i = 0; i < selectedObjects->size(); i++)
	{
		bin::debugBinType(selectedObjects->at(i));
	}*/

	//store zoomed data as bin structure
	m_zoomedRowData = bin::DeepCopy(thisZoomedRowData);

	auto tuple = std::make_tuple(thisZoomedRowData, selectedObjects);
	return tuple;
}

void iACompHistogramTable::highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId)
{
	vtkSmartPointer<vtkAlgorithm> algorithm = pickedActor->GetMapper()->GetInputConnection(0, 0)->GetProducer();
	vtkSmartPointer<vtkPlaneSource> oldPlane = vtkPlaneSource::SafeDownCast(algorithm);

	vtkSmartPointer<vtkIdTypeArray> ids = vtkSmartPointer<vtkIdTypeArray>::New();
	ids->SetNumberOfComponents(1);
	ids->InsertNextValue(pickedCellId);

	vtkSmartPointer<vtkSelectionNode> selectionNode = vtkSmartPointer<vtkSelectionNode>::New();
	selectionNode->SetFieldType(vtkSelectionNode::CELL);
	selectionNode->SetContentType(vtkSelectionNode::INDICES);
	selectionNode->SetSelectionList(ids);

	vtkSmartPointer<vtkSelection> selection = vtkSmartPointer<vtkSelection>::New();
	selection->AddNode(selectionNode);
	vtkSmartPointer<vtkExtractSelection> extractSelection = vtkSmartPointer<vtkExtractSelection>::New();
	extractSelection->SetInputData(0, oldPlane->GetOutputDataObject(0));
	extractSelection->SetInputData(1, selection);
	extractSelection->Update();

	vtkSmartPointer<vtkUnstructuredGrid> selected = vtkSmartPointer<vtkUnstructuredGrid>::New();
	selected->ShallowCopy(extractSelection->GetOutput());
	vtkSmartPointer<vtkDataSetMapper> selectedMapper = vtkSmartPointer<vtkDataSetMapper>::New();
	selectedMapper->SetInputData(selected);
	vtkSmartPointer<vtkActor> selectedActor = vtkSmartPointer<vtkActor>::New();
	selectedActor->SetMapper(selectedMapper);
	selectedActor->GetProperty()->EdgeVisibilityOn();
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN,col);

	selectedActor->GetProperty()->SetEdgeColor(col[0],col[1],col[2]);
	selectedActor->GetProperty()->SetLineWidth(iACompVisOptions::LINE_WIDTH);

	m_highlighingActors->push_back(selectedActor);
	m_renderer->AddActor(selectedActor);

	renderWidget();
}

void iACompHistogramTable::removeHighlightedCells()
{
	for (int i = 0; i < ((int)m_highlighingActors->size()); i++)
	{
		m_renderer->RemoveActor(m_highlighingActors->at(i));
	}

	m_highlighingActors->clear();
}

void iACompHistogramTable::highlightSelectedRow(vtkSmartPointer<vtkActor> pickedActor)
{
	vtkSmartPointer<vtkAlgorithm> algorithm = pickedActor->GetMapper()->GetInputConnection(0, 0)->GetProducer();
	vtkSmartPointer<vtkPlaneSource> plane = vtkPlaneSource::SafeDownCast(algorithm);

	//double xMin = plane->GetOrigin()[0];
	double xMax = plane->GetPoint1()[0];
	//double yMin = plane->GetOrigin()[1];
	double yMax = plane->GetPoint2()[1];

	double rightUpperPoint[3] = { xMax , yMax, 0 };

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	points->InsertNextPoint(plane->GetOrigin());
	points->InsertNextPoint(plane->GetPoint2());
	points->InsertNextPoint(rightUpperPoint);
	points->InsertNextPoint(plane->GetPoint1());
	points->InsertNextPoint(plane->GetOrigin());

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN,col);

	m_highlightRowActor = drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);

	renderWidget();
}

bool iACompHistogramTable::removeHighlightedRow()
{
	if(m_renderer->HasViewProp(m_highlightRowActor))
	{
		m_renderer->RemoveActor(m_highlightRowActor);
		return true;
	}

	return false;
}

void iACompHistogramTable::reorderHistogramTable(vtkSmartPointer<vtkActor> movingActor)
{
	//calculate in which row the moving actor will be set
	m_newDrawingPosition = calculateCurrentPosition(movingActor);

	if(m_oldDrawingPosition != -1)
	{
		//reorder the indices that describe the drawing order
		reorderIndices(m_newDrawingPosition, m_oldDrawingPosition);
	}
}

void iACompHistogramTable::reorderIndices(int newDrawingPos, int oldDrawingPos)
{
	//initialize storage for indices during moving of actor
	iACompVisOptions::copyVector(m_orderOfIndicesDatasets, m_newOrderOfIndicesDatasets);

	int difference = newDrawingPos - oldDrawingPos;

	if(difference > 0)
	{//reorder when difference is positive --> actor is moving up
		
		int currDataIndx = m_newOrderOfIndicesDatasets->at(newDrawingPos);
		m_newOrderOfIndicesDatasets->at(newDrawingPos) = m_newOrderOfIndicesDatasets->at(oldDrawingPos);

		for(int i = 1; i <= difference; i++)
		{	
			int nextDataIndx = m_newOrderOfIndicesDatasets->at(newDrawingPos - i);
			m_newOrderOfIndicesDatasets->at(newDrawingPos - i) = currDataIndx;
			currDataIndx = nextDataIndx;
		}
	}
	else if(difference < 0)
	{//reorder when difference is negative --> actor is moving down

		int currDataIndx = m_newOrderOfIndicesDatasets->at(newDrawingPos);
		m_newOrderOfIndicesDatasets->at(newDrawingPos) = m_newOrderOfIndicesDatasets->at(oldDrawingPos);

		for (int i = 1; i <= std::abs(difference); i++)
		{
			int nextDataIndx = m_newOrderOfIndicesDatasets->at(newDrawingPos + i);
			m_newOrderOfIndicesDatasets->at(newDrawingPos + i) = currDataIndx;
			currDataIndx = nextDataIndx;
		}
	}
}

void iACompHistogramTable::calculateOldDrawingPositionOfMovingActor(vtkSmartPointer<vtkActor> movingActor)
{
	//get oldPosition of the moving actor
	std::vector<vtkSmartPointer<vtkActor>>::iterator it = std::find(m_originalPlaneActors->begin(), m_originalPlaneActors->end(), movingActor);
	m_oldDrawingPosition = std::distance(m_originalPlaneActors->begin(), it);
}

int iACompHistogramTable::calculateCurrentPosition(vtkSmartPointer<vtkActor> movingActor)
{
	double yPos = movingActor->GetCenter()[1];

	std::map<int, std::vector<double>>::iterator iter = m_drawingPositionForRegions->begin();
	
	double minPossibleY = std::numeric_limits<double>::infinity();
	double maxPossibleY = -std::numeric_limits<double>::infinity();

	//inside defined drawing area
	while( iter != m_drawingPositionForRegions->end())
	{
		std::vector<double> yInterval = iter->second;
		double yMin = yInterval.at(0);
		double yMax = yInterval.at(1);

		if(yPos >= yMin && yPos <= yMax)
		{
			return iter->first;
		}

		if (minPossibleY > yMin) minPossibleY = yMin;
		if (maxPossibleY < yMax) maxPossibleY = yMax;

		iter++;
	}

	//outside defined drawing area
	if(yPos < minPossibleY)
	{
		return 0;
	}

	if (yPos > maxPossibleY) 
	{
		return m_amountDatasets - 1;
	}

	return -1;
}

/******************************************  Update  **********************************************/

void iACompHistogramTable::showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType)
{
	//remove the old selection
	removeSelectionOfCorrelationMap();

	for (std::map<int, double>::iterator iter = dataIndxSelectedType->begin(); iter != dataIndxSelectedType->end(); iter++)
	{
		int selectedDataInd = iter->first;

		std::map<vtkSmartPointer<vtkActor>, int>::iterator iterActor;
		for (iterActor = m_rowDataIndexPair->begin(); iterActor != m_rowDataIndexPair->end(); iterActor++)
		{ //find actor according to dataIndex
			
			int dataInd = iterActor->second;
			
			if(dataInd == selectedDataInd)
			{
				vtkSmartPointer<vtkActor> selectedAct = iterActor->first;
			
				vtkSmartPointer<vtkAlgorithm> algorithm = selectedAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
				vtkSmartPointer<vtkPlaneSource> oldPlane = vtkPlaneSource::SafeDownCast(algorithm);

				if (iter->second == 0.0)
				{ // is outer arc --> highlight whole dataset

					double col[3];
					iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY,col);

					drawStippledTexture(oldPlane->GetOrigin(), oldPlane->GetPoint1(), oldPlane->GetPoint2(), col);

				}else if(iter->second == 1.0)
				{ // is selected inner arc --> highlight selected cells
					
					std::map<int, std::vector<vtkIdType>*>::iterator pos = m_pickedCellsforPickedRow->find(selectedDataInd);
					if (pos != m_pickedCellsforPickedRow->end())
					{
						std::vector<vtkIdType>* cells = pos->second;
						
						double startX = oldPlane->GetOrigin()[0];
						double cellWidth = (oldPlane->GetPoint1()[0] - oldPlane->GetOrigin()[0]) / m_bins;

						for (int i = 0; i < ((int)cells->size()); i++)
						{
							vtkIdType cellId = cells->at(i);
							
							double startXCell = startX + (cellWidth * cellId);
							double endXCell = startXCell + cellWidth;

							double origin[3] = { startXCell, oldPlane->GetOrigin()[1], oldPlane->GetOrigin()[2] };
							double point1[3] = { endXCell, oldPlane->GetPoint1()[1], oldPlane->GetPoint1()[2] };
							double point2[3] = { startXCell, oldPlane->GetPoint2()[1], oldPlane->GetPoint2()[2] };

							double col[3];
							iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN,col);

							drawStippledTexture(origin, point1, point2, col);
						}
					}
				}
				else
				{ //is not-selected inner arc --> highlight all other cells (not the selected ones)
					std::map<int, std::vector<vtkIdType>*>::iterator pos = m_pickedCellsforPickedRow->find(selectedDataInd);
					if (pos != m_pickedCellsforPickedRow->end())
					{
						std::vector<vtkIdType>* cells = pos->second;
						std::vector<vtkIdType>::iterator cellIterator;

						double startX = oldPlane->GetOrigin()[0];
						double cellWidth = (oldPlane->GetPoint1()[0] - oldPlane->GetOrigin()[0]) / m_bins;

						for(int i = 0; i < m_bins; i++)
						{
							cellIterator = std::find(cells->begin(), cells->end(), i); 
							if (cellIterator == cells->end())
							{ //when the cell is not one of the selected ones
								double startXCell = startX + (cellWidth * i);
								double endXCell = startXCell + cellWidth;

								double origin[3] = { startXCell, oldPlane->GetOrigin()[1], oldPlane->GetOrigin()[2] };
								double point1[3] = { endXCell, oldPlane->GetPoint1()[1], oldPlane->GetPoint1()[2] };
								double point2[3] = { startXCell, oldPlane->GetPoint2()[1], oldPlane->GetPoint2()[2] };

								double col[3];
								iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTERGREY,col);

								drawStippledTexture(origin, point1, point2, col);
							}
						}
					}
				}
			}
		}
	}

	renderWidget();

}

void iACompHistogramTable::drawStippledTexture(double* origin, double* point1, double* point2, double* color)
{
	vtkSmartPointer<vtkPlaneSource> texturePlane = vtkSmartPointer<vtkPlaneSource>::New();
	texturePlane->SetXResolution(1);
	texturePlane->SetYResolution(1);
	texturePlane->SetOrigin(origin);
	texturePlane->SetPoint1(point1);
	texturePlane->SetPoint2(point2);

	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(texturePlane->GetOutputPort());
	mapper->Update();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	actor->GetProperty()->SetLineWidth(iACompVisOptions::LINE_WIDTH);

	actor->GetProperty()->SetColor(color[0], color[1], color[2]);
	iACompVisOptions::stippledLine(actor, 0xAAAA, 1);

	m_renderer->AddActor(actor);
	m_stippledActors->push_back(actor);
}

void iACompHistogramTable::removeSelectionOfCorrelationMap()
{
	for(int i = 0; i < ((int)m_stippledActors->size()); i++)
	{
		m_renderer->RemoveActor(m_stippledActors->at(i));
	}

	m_stippledActors->clear();
}
