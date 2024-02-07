// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompUniformTable.h"

//CompVis
#include "iACompUniformBinningData.h"
#include "iACompHistogramVis.h"

//vtk
#include <vtkActor.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkGlyph2D.h>
#include <vtkLineSource.h>
#include <vtkLookupTable.h>
#include <vtkNamedColors.h>
#include <vtkPlaneSource.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProgrammableGlyphFilter.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTextActor.h>
#include <vtkUnsignedCharArray.h>

iACompUniformTable::iACompUniformTable(iACompHistogramVis* vis, iACompUniformBinningData* uniformBinningData) :
	iACompTable(vis), 
	m_uniformBinningData(uniformBinningData),
	m_originalPlaneActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_zoomedPlaneActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	originalPlaneZoomedPlanePair(new std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>*>()),
	m_highlightRowActor(vtkSmartPointer<vtkActor>::New()),
	m_pointRepresentationActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_stippledActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_oldDrawingPosition(-1),
	m_newDrawingPosition(-1),
	m_interactionStyle(vtkSmartPointer<iACompUniformTableInteractorStyle>::New())
{
	m_bins = m_uniformBinningData->getInitialNumberOfBins();
	m_binsZoomed = m_bins;
	minBins = m_bins;

	//initialize interaction
	initializeInteraction();

	//initialize visualization
	initializeTable();
}

void iACompUniformTable::initializeTable()
{
	//the color scheme is depending on the uniform binning
	calculateBinRange();

	//setup color table
	makeLUTFromCTF();
	makeLUTDarker();

	//initialize legend
	initializeLegend();
}

void iACompUniformTable::initializeInteraction()
{
	m_interactionStyle->setUniformTableVisualization(this);
	m_interactionStyle->SetDefaultRenderer(m_mainRenderer);
	m_interactionStyle->setIACompHistogramVis(m_vis);
}

void iACompUniformTable::setActive()
{
	//add rendererColorLegend to widget
	addLegendRendererToWidget();

	if (m_lastState == iACompVisOptions::lastState::Undefined)
	{
		//init camera
		initializeCamera();

		drawHistogramTable(m_bins);

		m_mainRenderer->ResetCamera();
		renderWidget();

		m_lastState = iACompVisOptions::lastState::Defined;
	}
	else if (m_lastState == iACompVisOptions::lastState::Defined)
	{
		m_interactionStyle->reinitializeState();

		drawHistogramTable(m_bins);
		
		//m_mainRenderer->ResetCamera();
		renderWidget();
	}
	else if (m_lastState == iACompVisOptions::lastState::Changed)
	{
		drawHistogramTable(m_bins);
		
		//m_mainRenderer->ResetCamera();
		renderWidget();
	}
}

void iACompUniformTable::setInactive()
{

}

void iACompUniformTable::initializeCamera()
{
	m_vis->setCamera(m_mainRenderer->GetActiveCamera());
}

/******************************************  Coloring (LookupTable)  **********************************/
void iACompUniformTable::calculateBinRange()
{
	int maxAmountInAllBins = m_uniformBinningData->getMaxAmountInAllBins();
	m_BinRangeLength = ((double)maxAmountInAllBins) / ((double)m_tableSize);
}

/*************** Rendering ****************************/

void iACompUniformTable::drawReorderedHistogramTable()
{
	iACompVisOptions::copyVector(m_vis->getNewOrderOfIndicesDatasets(), m_vis->getOrderOfIndicesDatasets());
	drawHistogramTable(m_bins);

	//highlight moved actor
	highlightSelectedRow(m_originalPlaneActors->at(m_newDrawingPosition));
}

void iACompUniformTable::drawHistogramTable(int bins)
{
	m_vis->calculateRowWidthAndHeight(m_vis->getWindowWidth(),m_vis->getWindowHeight(),m_vis->getAmountDatasets());
	
	if (m_mainRenderer->GetViewProps()->GetNumberOfItems() > 0)
	{
		m_mainRenderer->RemoveAllViewProps();
		m_uniformBinningData->resetBinPolyData();
	}
	m_originalPlaneActors->clear();
	m_zoomedPlaneActors->clear();
	m_rowDataIndexPair->clear();

	//draw cells from bottom to top --> so start with last dataset and go to first
	for (int currCol = 0; currCol < m_vis->getAmountDatasets(); currCol++)
	{
		int dataInd = m_vis->getOrderOfIndicesDatasets()->at(currCol);
		vtkSmartPointer<vtkPolyData> row = drawRow(dataInd, currCol, bins, 0);
		m_uniformBinningData->storeBinPolyData(row);
	}

	//draw x-axis on the bottom
	double min_x = 0.0;
	double max_x = m_vis->getRowSize();
	double max_y = m_vis->getColSize() * -0.25;
	double min_y = m_vis->getColSize() * -0.75;
	double drawingDimensions[4] = {min_x, max_x, min_y, max_y};
	drawXAxis(drawingDimensions);

	renderWidget();
}

vtkSmartPointer<vtkPolyData> iACompUniformTable::drawRow(int currDataInd, int currentColumn, int amountOfBins, double offset)
{
	Q_UNUSED(amountOfBins);
	bin::BinType* currDataset = m_uniformBinningData->getBinData()->at(currDataInd);
	double numberOfBins = currDataset->size();

	vtkSmartPointer<vtkPolyData> row = initializeRow(numberOfBins);
	vtkSmartPointer<vtkDoubleArray> originArray = static_cast<vtkDoubleArray*>(row->GetPointData()->GetArray("originArray"));
	vtkSmartPointer<vtkDoubleArray> point1Array = static_cast<vtkDoubleArray*>(row->GetPointData()->GetArray("point1Array"));
	vtkSmartPointer<vtkDoubleArray> point2Array = static_cast<vtkDoubleArray*>(row->GetPointData()->GetArray("point2Array"));
	vtkSmartPointer<vtkUnsignedCharArray> colorArray = static_cast<vtkUnsignedCharArray*>(row->GetCellData()->GetArray("colorArray"));

	//drawing positions
	double min_x = 0.0;
	double max_x = m_vis->getRowSize();
	double min_y = (m_vis->getColSize() * currentColumn) + offset;
	double max_y = min_y + m_vis->getColSize();

	this->constructBins(m_uniformBinningData, currDataset, originArray, point1Array, point2Array, colorArray,
		currDataInd,
		currentColumn, offset);

	vtkSmartPointer<vtkPlaneSource> planeSource = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
	planeSource->SetCenter(0, 0, 0);
	planeSource->Update();

	vtkSmartPointer<vtkProgrammableGlyphFilter> glypher = vtkSmartPointer<vtkProgrammableGlyphFilter>::New();
	glypher->SetInputData(row);
	glypher->SetSourceData(planeSource->GetOutput());
	glypher->SetGlyphMethod(buildGlyphRepresentation, glypher);
	glypher->Update();

	vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	glyphMapper->SetInputConnection(glypher->GetOutputPort());
	glyphMapper->SetColorModeToDefault();
	glyphMapper->SetScalarModeToUseCellData();
	glyphMapper->GetInput()->GetCellData()->SetScalars(colorArray);
	glyphMapper->ScalarVisibilityOn();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(glyphMapper);
	if (!m_useDarkerLut)
	{  //the edges of the cells are drawn
		actor->GetProperty()->EdgeVisibilityOn();
		double col[3];
		iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_BLACK, col);
		actor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
		actor->GetProperty()->SetLineWidth(actor->GetProperty()->GetLineWidth() * 1.5);
	}
	else
	{  //not showing the edges of the cells
		actor->GetProperty()->EdgeVisibilityOff();
	}

	m_mainRenderer->AddActor(actor);

	m_originalPlaneActors->push_back(actor);
	//store row and for each row the index which dataset it is showing
	m_rowDataIndexPair->insert({actor, currDataInd});

	double y = (m_vis->getColSize() * currentColumn) + offset;

	//add name of dataset/row
	double pos[3] = { -(m_vis->getRowSize()) * 0.05, y + (m_vis->getColSize()* 0.5), 0.0 };
	addDatasetName(currDataInd, pos);

	//add X Ticks
	if (m_vis->getXAxis())
	{
		double drawingDimensions[4] = {min_x, max_x, min_y, max_y};
		double yheight = min_y;
		double tickLength = (max_y - min_y) * 0.05;
		drawXTicks(drawingDimensions, yheight, tickLength);
	}

	return row;
}

void iACompUniformTable::colorBinOfRow(vtkUnsignedCharArray* colorArray, int position, double numberOfObjectsInBin)
{
	double* rgbBorder;
	double rgb[3] = {0.0, 0.0, 0.0};
	double maxNumber = m_lut->GetRange()[1];
	double minNumber = m_lut->GetRange()[0];

	if (numberOfObjectsInBin > maxNumber)
	{
		if (m_useDarkerLut)
		{
			rgbBorder = m_lutDarker->GetAboveRangeColor();
		}
		else
		{
			rgbBorder = m_lut->GetAboveRangeColor();
		}

		unsigned char ucrgb[3];
		iACompVisOptions::getColorArray3(rgbBorder, ucrgb);
		colorArray->InsertTuple3(position, ucrgb[0], ucrgb[1], ucrgb[2]);
	}
	else if (numberOfObjectsInBin < minNumber)
	{
		if (m_useDarkerLut)
		{
			rgbBorder = m_lutDarker->GetBelowRangeColor();
		}
		else
		{
			rgbBorder = m_lut->GetBelowRangeColor();
		}

		unsigned char ucrgb[3];
		iACompVisOptions::getColorArray3(rgbBorder, ucrgb);
		colorArray->InsertTuple3(position, ucrgb[0], ucrgb[1], ucrgb[2]);
	}
	else
	{
		if (m_useDarkerLut)
		{
			m_lutDarker->GetColor(numberOfObjectsInBin, rgb);
		}
		else
		{
			m_lut->GetColor(numberOfObjectsInBin, rgb);
		}

		unsigned char ucrgb[3];
		iACompVisOptions::getColorArray3(rgb, ucrgb);
		colorArray->InsertTuple3(position, ucrgb[0], ucrgb[1], ucrgb[2]);
	}
}

vtkSmartPointer<vtkActor> iACompUniformTable::drawPolyLine(vtkSmartPointer<vtkPoints> points, double lineColor[3], double lineWidth)
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

	m_mainRenderer->AddActor(polyLineActor);

	return polyLineActor;
}

void iACompUniformTable::drawHistogramTableAccordingToSimilarity(vtkSmartPointer<vtkActor> referenceData)
{
	//get for all dataset each bin with its MDS values
	QList<bin::BinType*>* binData = m_uniformBinningData->getBinData();

	//set dataIndex
	std::map<vtkSmartPointer<vtkActor>, int>::iterator it = m_rowDataIndexPair->find(referenceData);
	if (it == m_rowDataIndexPair->end())
	{
		return;
	}
	int dataIndex = it->second;

	bin::BinType* referenceRow = binData->at(dataIndex);
	std::vector<double> results = std::vector<double>(m_vis->getAmountDatasets(), 0);

	for (int currInd = 0; currInd < m_vis->getAmountDatasets(); currInd++)
	{
		if (currInd == dataIndex)
		{  //there is no difference in the same dataset
			results.at(currInd) = 0.0;
		}
		else
		{
			bin::BinType* testRow = binData->at(currInd);
			double result = calculateChiSquaredMetric(testRow, referenceRow);
			results.at(currInd) = result;
		}
	}

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = m_vis->sortWithMemory(results, 0);
	m_vis->setOrderOfIndicesDatasets(m_vis->reorderAccordingTo(newOrder));

	drawHistogramTable(m_bins);

	//highlight row which was used as reference
	highlightSelectedRow(m_originalPlaneActors->at(m_vis->getAmountDatasets() - 1));
}

void iACompUniformTable::drawHistogramTableAccordingToCellSimilarity(Pick::PickedMap* m_picked)
{
	//get for all dataset each bin with its MDS values
	QList<bin::BinType*>* binData = m_uniformBinningData->getBinData();
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
	if (it == m_rowDataIndexPair->end())
	{
		return;
	}
	int dataIndex = it->second;

	//get all the bins
	bin::BinType* referenceRow = binData->at(dataIndex);
	bin::BinType* referenceCells = bin::copyCells(referenceRow, indexOfCells);

	std::vector<double> results = std::vector<double>(m_vis->getAmountDatasets(), 0);

	for (int currInd = 0; currInd < m_vis->getAmountDatasets(); currInd++)
	{
		if (currInd == dataIndex)
		{  //there is no difference in the same dataset
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
	std::vector<int>* newOrder = m_vis->sortWithMemory(results, 0);
	m_vis->setOrderOfIndicesDatasets(m_vis->reorderAccordingTo(newOrder));

	drawHistogramTable(m_bins);

	//highlight selected cells
	for (int i = 0; i < static_cast<int>(indexOfCells->size()); i++)
	{
		highlightSelectedCell(m_originalPlaneActors->at(m_vis->getAmountDatasets() - 1), indexOfCells->at(i));
	}
}

void iACompUniformTable::drawNonLinearZoom(
	Pick::PickedMap* map, int notSelectedBinNumber, int selectedBinNumber, QList<bin::BinType*>* zoomedRowData)
{
	QList<bin::BinType*>* thisZoomedRowData = bin::DeepCopy(zoomedRowData);

	//draw zoomed histogram table new
	m_mainRenderer->RemoveAllViewProps();
	m_originalPlaneActors->clear();
	m_zoomedPlaneActors->clear();
	originalPlaneZoomedPlanePair->clear();
	m_rowDataIndexPair->clear();

	int currCol = 0;
	double offset = 0;
	double distance = 0.03;           //distance to other rows in percent [0,1]
	double distanceToParent = 0.015;  //distance to row from which it is the zoom representation in percent [0,1]
	bool addedRow = false;
	bool offsetAlreadyAdded = false;

	double windowHeight = m_vis->getWindowHeight();
	double windowWidth = m_vis->getWindowWidth();
	//double newHeight = windowHeight - (((windowHeight * distance) * 4) * map->size()) -
	//	(((windowHeight * distanceToParent)) * map->size());
	double newHeight = windowHeight - (((windowHeight * distance) ) * map->size()) - (((windowHeight * distanceToParent)) * map->size()); //////////////////////////////////

	m_vis->calculateRowWidthAndHeight(windowWidth, newHeight, m_vis->getAmountDatasets() + map->size());

	std::vector<vtkSmartPointer<vtkPolyData>>* zoomedPlanes = new std::vector<vtkSmartPointer<vtkPolyData>>();
	vtkSmartPointer<vtkPolyData> originalPlane;

	for (int counter = 0; counter < m_vis->getAmountDatasets(); counter++)
	{
		int indData = m_vis->getOrderOfIndicesDatasets()->at(counter);

		//check for additional row
		std::vector<int>::iterator it = std::find(m_indexOfPickedRow->begin(), m_indexOfPickedRow->end(), indData);
		if (it != m_indexOfPickedRow->end())
		{
			if (currCol != 0 && !offsetAlreadyAdded)
			{  //do not add a offset when the zoomed row is the undermost
				offset = offset + distance;
			}

			//draw zoomed dataset
			std::map<int, std::vector<vtkIdType>*>::const_iterator pos = m_pickedCellsforPickedRow->find(indData);
			std::vector<vtkIdType>* cellIds = pos->second;
			zoomedPlanes = drawZoomedRow(currCol, selectedBinNumber, thisZoomedRowData->last(), offset, cellIds);
			thisZoomedRowData->removeLast();

			currCol += 1;
			addedRow = true;
			offset = offset + distanceToParent + (m_vis->getColSize() * 0.5);  // zoomed rows are 2*m_colSize high
		}
		offsetAlreadyAdded = false;

		//draw original datasets
		originalPlane = drawRow(indData, currCol, notSelectedBinNumber, offset);

		if (addedRow)
		{
			//do for each cell of the selected row
			std::map<int, std::vector<vtkIdType>*>::const_iterator pos = m_pickedCellsforPickedRow->find(indData);
			if (pos != m_pickedCellsforPickedRow->end())
			{
				std::vector<vtkIdType>* cellIds = pos->second;
				vtkSmartPointer<vtkActor> thisAcc = m_mainRenderer->GetActors()->GetLastActor();

				for (int i = 0; i < static_cast<int>(cellIds->size()); i++)
				{  //highlight each cell of the selected row

					highlightSelectedCell(thisAcc, cellIds->at(i));
				}

				//draw line border
				drawLineBetweenRowAndZoomedRow(zoomedPlanes, originalPlane, cellIds);
			}

			offset = offset + distance;
			addedRow = false;
			offsetAlreadyAdded = true;

			//store for each original row its zoomed row actors
			std::vector<vtkSmartPointer<vtkActor>>* zoomedActorsForThisRow = new std::vector<vtkSmartPointer<vtkActor>>();
			for (int i = (((int)zoomedPlanes->size()) - 1); i >= 0; i--)
			{
				zoomedActorsForThisRow->push_back(m_zoomedPlaneActors->at((m_zoomedPlaneActors->size() - 1) - i));
			}
			originalPlaneZoomedPlanePair->insert(
				{m_originalPlaneActors->at(m_originalPlaneActors->size() - 1), zoomedActorsForThisRow});
		}
		else
		{
			vtkSmartPointer<vtkActor> thisAcc = m_mainRenderer->GetActors()->GetLastActor();
			//set opacity for not selected original rows
			thisAcc->GetProperty()->SetOpacity(0.6);
		}

		currCol += 1;
	}

	renderWidget();

	m_mainRenderer->ResetCamera();
	renderWidget();
}

std::vector<vtkSmartPointer<vtkPolyData>>* iACompUniformTable::drawZoomedRow(int currentColumn,
	int amountOfBins, bin::BinType* currentData, double offsetHeight, std::vector<vtkIdType>* cellIdsOriginalPlane)
{
	std::vector<vtkSmartPointer<vtkPolyData>>* zoomedPlanes = new std::vector<vtkSmartPointer<vtkPolyData>>();

	double selectedCellsWithObjects = 0;
	for (int k = 0; k < static_cast<int>(currentData->size()); k++)
	{
		if (currentData->at(k).size() > 1)
		{
			selectedCellsWithObjects = selectedCellsWithObjects + 1;
		}
	}

	double rowSize = m_vis->getRowSize();
	double colSize = m_vis->getColSize();

	double widthOriginal = rowSize + rowSize * 0.7;
	double widthRange = widthOriginal / selectedCellsWithObjects;
	double offsetWidth = rowSize * 0.05;

	double startX = -rowSize * 0.35;
	double endX = startX + widthRange;
	double startY = (colSize * currentColumn) + offsetHeight;
	double endY = startY + (colSize);  //startY + (1.5 * colSize); ///////////////////////////////

	for (int i = 0; i < static_cast<int>(cellIdsOriginalPlane->size()); i++)
	{
		vtkSmartPointer<vtkPolyData> plane = drawZoomedPlane(amountOfBins, startX, startY, endX, endY, i, currentData); 
		if (plane == nullptr)
		{
			continue;
		}

		zoomedPlanes->push_back(plane);

		if (i < (static_cast<int>(cellIdsOriginalPlane->size()) - 1))
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

void iACompUniformTable::zoomInZoomedRow(int selectedBinNumber)
{
	int rowId = m_zoomedRowData->size() - 1;
	int cellId = 0;
	for (int zoomedRowDataInd = 0; zoomedRowDataInd < static_cast<int>(m_zoomedPlaneActors->size()); zoomedRowDataInd++)
	{  //for all zoomed planes

		vtkSmartPointer<vtkActor> currAct = m_zoomedPlaneActors->at(zoomedRowDataInd);

		bin::BinType* currData = m_zoomedRowData->at(rowId);
		drawZoomForZoomedRow(currAct, zoomedRowDataInd, currData, selectedBinNumber);

		//get the correct data bins of zoomedRowData
		if (cellId >= (static_cast<int>(m_zoomedRowData->at(rowId)->size()) - 1))
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

void iACompUniformTable::drawZoomForZoomedRow(
	vtkSmartPointer<vtkActor> zoomedRowActor, int currBin, bin::BinType* data, int amountOfBins)
{
	vtkSmartPointer<vtkAlgorithm> algorithm = zoomedRowActor->GetMapper()->GetInputConnection(0, 0)->GetProducer();
	vtkSmartPointer<vtkPolyDataAlgorithm> polyDataAlgorithm = vtkPolyDataAlgorithm::SafeDownCast(algorithm);
	vtkSmartPointer<vtkPolyData> zoomedRow = polyDataAlgorithm->GetPolyDataInput(0);

	m_vis->calculateSpecificBins(iACompVisOptions::binningType::Uniform, data, currBin, amountOfBins);
	bin::BinType* binData = m_uniformBinningData->getZoomedBinData();

	if (binData == nullptr) return;
	
	std::vector<double>* min_max = bin::getMinimumAndMaximum(binData);
	double minValueInBin = min_max->at(0);
	double maxValueInBin = min_max->at(1);
	std::vector<double> binBoundaries =
		iACompVisOptions::calculateBinBoundaries(minValueInBin, maxValueInBin, amountOfBins);

	vtkSmartPointer<vtkDoubleArray> origA= static_cast<vtkDoubleArray*>(zoomedRow->GetPointData()->GetArray("originArray"));
	vtkSmartPointer<vtkDoubleArray> point1A = static_cast<vtkDoubleArray*>(zoomedRow->GetPointData()->GetArray("point1Array"));
	vtkSmartPointer<vtkDoubleArray> point2A = static_cast<vtkDoubleArray*>(zoomedRow->GetPointData()->GetArray("point2Array"));

	double originZoomed[3];
	origA->GetTuple(0, originZoomed);

	double point1Zoomed[3];
	point1A->GetTuple(point1A->GetNumberOfTuples() - 1, point1Zoomed);
	
	double point2Zoomed[3];
	point2A->GetTuple(0, point2Zoomed);
	
	double startX = originZoomed[0];
	double startY = originZoomed[1];
	double endX = point1Zoomed[0];
	double endY = point2Zoomed[1];

	//compute visual encoding - zoomed planes
	vtkSmartPointer<vtkPolyData> row = initializeRow(amountOfBins);
	vtkSmartPointer<vtkDoubleArray> originArray =
		static_cast<vtkDoubleArray*>(row->GetPointData()->GetArray("originArray"));
	vtkSmartPointer<vtkDoubleArray> point1Array =
		static_cast<vtkDoubleArray*>(row->GetPointData()->GetArray("point1Array"));
	vtkSmartPointer<vtkDoubleArray> point2Array =
		static_cast<vtkDoubleArray*>(row->GetPointData()->GetArray("point2Array"));
	vtkSmartPointer<vtkUnsignedCharArray> colorArray =
		static_cast<vtkUnsignedCharArray*>(row->GetCellData()->GetArray("colorArray"));

	for (int i = 0; i < amountOfBins; i++)
	{
		//compute bin boundaries in drawing coordinates
		double minBoundary = binBoundaries.at(i);
		double xMin = iACompVisOptions::histogramNormalization(minBoundary, startX, endX, minValueInBin, maxValueInBin);

		double maxBoundary = maxValueInBin;
		if (i != (amountOfBins - 1))
		{
			maxBoundary = binBoundaries.at(i + 1);
		}
		double xMax = iACompVisOptions::histogramNormalization(maxBoundary, startX, endX, minValueInBin, maxValueInBin);

		//set position coordinates for each bin glyph
		originArray->InsertTuple3(i, xMin, startY, 0.0);
		point1Array->InsertTuple3(i, xMax, startY, 0.0);  //width
		point2Array->InsertTuple3(i, xMin, endY, 0.0);    //height

		//set color for each bin glyph
		if (binData == nullptr)
		{
			double rgb[3];
			m_lut->GetColor(-1, rgb);
			unsigned char ucrgb[3];
			iACompVisOptions::getColorArray3(rgb, ucrgb);
			colorArray->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
		}
		else
		{
			double numberOfObjects = binData->at(i).size();
			colorBinOfRow(colorArray, i, numberOfObjects);
		}

		originArray->Modified();
		point1Array->Modified();
		point2Array->Modified();
		colorArray->Modified();
		row->Modified();
	}

	vtkSmartPointer<vtkPlaneSource> planeSource = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
	planeSource->SetCenter(0, 0, 0);
	planeSource->Update();

	vtkSmartPointer<vtkProgrammableGlyphFilter> glypher = vtkSmartPointer<vtkProgrammableGlyphFilter>::New();
	glypher->SetInputData(row);
	glypher->SetSourceData(planeSource->GetOutput());
	glypher->SetGlyphMethod(buildGlyphRepresentation, glypher);
	glypher->Update();

	vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	glyphMapper->SetInputConnection(glypher->GetOutputPort());
	glyphMapper->SetColorModeToDefault();
	glyphMapper->SetScalarModeToUseCellData();
	glyphMapper->GetInput()->GetCellData()->SetScalars(colorArray);
	glyphMapper->ScalarVisibilityOn();
	
	zoomedRowActor->SetMapper(glyphMapper);
	zoomedRowActor->Modified();
}

vtkSmartPointer<vtkPolyData> iACompUniformTable::drawZoomedPlane(
	int bins, double startX, double startY, double endX, double endY, int currBinIndex, bin::BinType* currentData)
{
	//compute zoomed bin data (MDS values)
	double minValueInBin;
	double maxValueInBin;

	std::vector<double> pickedBinData = currentData->at(currBinIndex);
	std::sort(pickedBinData.begin(), pickedBinData.end());
	
	m_vis->calculateSpecificBins(iACompVisOptions::binningType::Uniform, currentData, currBinIndex, bins);
	bin::BinType* binData = m_uniformBinningData->getZoomedBinData();

	if (pickedBinData.size() == 0)
	{
		LOG(lvlInfo, "Empty Bins cannot be inspected in more detail.");
		return nullptr;
	}
	else if (pickedBinData.size() == 1)
	{
		LOG(lvlInfo, "Bins containing one object cannot be inspected in more detail.");
		return nullptr;
	}
	else if (pickedBinData.size() == 2)
	{
		minValueInBin = pickedBinData.at(0);
		maxValueInBin = pickedBinData.at(1);
	}
	else
	{
		minValueInBin = pickedBinData.at(0);
		maxValueInBin = pickedBinData.at(pickedBinData.size() - 1);
	}
	
	std::vector<double> binBoundaries = iACompVisOptions::calculateBinBoundaries(minValueInBin, maxValueInBin, bins);

	//compute visual encoding - zoomed planes
	vtkSmartPointer<vtkPolyData> row = initializeRow(bins);
	vtkSmartPointer<vtkDoubleArray> originArray = static_cast<vtkDoubleArray*>(row->GetPointData()->GetArray("originArray"));
	vtkSmartPointer<vtkDoubleArray> point1Array = static_cast<vtkDoubleArray*>(row->GetPointData()->GetArray("point1Array"));
	vtkSmartPointer<vtkDoubleArray> point2Array = static_cast<vtkDoubleArray*>(row->GetPointData()->GetArray("point2Array"));
	vtkSmartPointer<vtkUnsignedCharArray> colorArray = static_cast<vtkUnsignedCharArray*>(row->GetCellData()->GetArray("colorArray"));

	for (int i = 0; i < bins; i++)
	{
		//compute bin boundaries in drawing coordinates
		double minBoundary = binBoundaries.at(i);
		double xMin = iACompVisOptions::histogramNormalization(minBoundary, startX, endX, minValueInBin, maxValueInBin);

		double maxBoundary = maxValueInBin;
		if (i != (bins - 1))
		{
			maxBoundary = binBoundaries.at(i+1);
		}
		double xMax = iACompVisOptions::histogramNormalization(maxBoundary, startX, endX, minValueInBin, maxValueInBin);
		
		//set position coordinates for each bin glyph
		originArray->InsertTuple3(i, xMin, startY, 0.0);
		point1Array->InsertTuple3(i, xMax, startY, 0.0);  //width
		point2Array->InsertTuple3(i, xMin, endY, 0.0);    //height
		
		//set color for each bin glyph
		if (binData == nullptr)
		{
			double rgb[3];
			m_lut->GetColor(-1, rgb);
			unsigned char ucrgb[3];
			iACompVisOptions::getColorArray3(rgb, ucrgb);
			colorArray->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
		}
		else
		{
			double numberOfObjects = binData->at(i).size();
			colorBinOfRow(colorArray, i, numberOfObjects);
		}

		originArray->Modified();
		point1Array->Modified();
		point2Array->Modified();
		colorArray->Modified();
		row->Modified();
	}

	vtkSmartPointer<vtkPlaneSource> planeSource = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
	planeSource->SetCenter(0, 0, 0);
	planeSource->Update();

	vtkSmartPointer<vtkProgrammableGlyphFilter> glypher = vtkSmartPointer<vtkProgrammableGlyphFilter>::New();
	glypher->SetInputData(row);
	glypher->SetSourceData(planeSource->GetOutput());
	glypher->SetGlyphMethod(buildGlyphRepresentation, glypher);
	glypher->Update();

	vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	glyphMapper->SetInputConnection(glypher->GetOutputPort());
	glyphMapper->SetColorModeToDefault();
	glyphMapper->SetScalarModeToUseCellData();
	glyphMapper->GetInput()->GetCellData()->SetScalars(colorArray);
	glyphMapper->ScalarVisibilityOn();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(glyphMapper);
	actor->GetProperty()->EdgeVisibilityOn();
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	actor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
	actor->GetProperty()->SetLineWidth(1);

	vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
	actor->GetProperty()->SetColor(nc->GetColor3d("DarkGray").GetData());
	m_mainRenderer->AddActor2D(actor);

	m_zoomedPlaneActors->push_back(actor);

	return row;
}

void iACompUniformTable::drawLineBetweenRowAndZoomedRow(std::vector<vtkSmartPointer<vtkPolyData>> * zoomedRowPlanes,
	vtkSmartPointer<vtkPolyData> originalRowPlane, std::vector<vtkIdType>* cellIdsOriginalPlane)
{
	vtkSmartPointer<vtkDoubleArray> originArray =
		static_cast<vtkDoubleArray*>(originalRowPlane->GetPointData()->GetArray("originArray"));
	vtkSmartPointer<vtkDoubleArray> point1Array =
		static_cast<vtkDoubleArray*>(originalRowPlane->GetPointData()->GetArray("point1Array"));

	for (int i = 0; i < static_cast<int>(zoomedRowPlanes->size()); i++)
	{
		vtkSmartPointer<vtkPolyData> zoomedRowPlane = zoomedRowPlanes->at(i);
		vtkSmartPointer<vtkDoubleArray> originA =
			static_cast<vtkDoubleArray*>(zoomedRowPlane->GetPointData()->GetArray("originArray"));
		vtkSmartPointer<vtkDoubleArray> point1A =
			static_cast<vtkDoubleArray*>(zoomedRowPlane->GetPointData()->GetArray("point1Array"));
		vtkSmartPointer<vtkDoubleArray> point2A =
			static_cast<vtkDoubleArray*>(zoomedRowPlane->GetPointData()->GetArray("point2Array"));

		double originZoomed[3];
		originA->GetTuple(0, originZoomed);

		double point1Zoomed[3];
		point1A->GetTuple(point1A->GetNumberOfTuples() - 1, point1Zoomed);

		double point2Zoomed[3];
		point2A->GetTuple(0, point2Zoomed);

		double xMinZ = originZoomed[0];
		double xMaxZ = point1Zoomed[0];
		double yMinZ = originZoomed[1];
		double yMaxZ = point2Zoomed[1];

		double currCellId = cellIdsOriginalPlane->at(i);

		double col[3];
		iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);

		//left line from selected cell to the left outer point of the zoomed row
		double origin[3];
		originArray->GetTuple(cellIdsOriginalPlane->at(i), origin);

		double point1[3];
		point1Array->GetTuple(cellIdsOriginalPlane->at(i), point1);

		double xMinO = origin[0];
		double yMinO = origin[1];
		double xMaxO = point1[0];

		double p0[3];
		p0[0] = xMinO;
		p0[1] = yMinO;
		p0[2] = 0.0;

		double p1[3];
		p1[0] = xMinZ;
		p1[1] = yMaxZ;
		p1[2] = 0.0;

		drawLine(p0, p1, col, iACompVisOptions::LINE_WIDTH);

		//right line from selected cell to the right outer point of the zoomed row 
		double p2[3];
		p2[0] = xMaxO;
		p2[1] = p0[1];
		p2[2] = p0[2];

		double p3[3];
		p3[0] = xMaxZ;
		p3[1] = p1[1];
		p3[2] = p1[2];

		drawLine(p2, p3, col, iACompVisOptions::LINE_WIDTH);

		//border zoomed row
		double leftDown[3] = {p1[0], yMinZ, 0.0};
		double rightDown[3] = {p3[0], yMinZ, 0.0};
		double rightUp[3] = {p3[0], p3[1], p3[2]};
		double leftUp[3] = {p1[0], p1[1], p1[2]};

		if (i == 0)
		{
			//left line
			vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
			points->InsertNextPoint(leftUp);
			points->InsertNextPoint(leftDown);
			drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);

			if (cellIdsOriginalPlane->size() > 1)
			{
				double nextCellId = cellIdsOriginalPlane->at(i + 1);
				if ((currCellId + 1) != nextCellId)
				{
					//right line
					vtkSmartPointer<vtkPoints> pointList = vtkSmartPointer<vtkPoints>::New();
					pointList->InsertNextPoint(rightUp);
					pointList->InsertNextPoint(rightDown);
					drawPolyLine(pointList, col, iACompVisOptions::LINE_WIDTH);
				}
			}
			else
			{  //right line
				vtkSmartPointer<vtkPoints> pointList = vtkSmartPointer<vtkPoints>::New();
				pointList->InsertNextPoint(rightUp);
				pointList->InsertNextPoint(rightDown);
				drawPolyLine(pointList, col, iACompVisOptions::LINE_WIDTH);
			}
		}
		else if (i == (static_cast<int>(cellIdsOriginalPlane->size()) - 1))
		{
			//right line
			vtkSmartPointer<vtkPoints> pointList = vtkSmartPointer<vtkPoints>::New();
			pointList->InsertNextPoint(rightUp);
			pointList->InsertNextPoint(rightDown);
			drawPolyLine(pointList, col, iACompVisOptions::LINE_WIDTH);

			double lastCellId = cellIdsOriginalPlane->at(i - 1);
			if ((currCellId - 1) != lastCellId)
			{
				//left line
				vtkSmartPointer<vtkPoints> points1 = vtkSmartPointer<vtkPoints>::New();
				points1->InsertNextPoint(leftUp);
				points1->InsertNextPoint(leftDown);
				drawPolyLine(points1, col, iACompVisOptions::LINE_WIDTH);
			}
		}
		else if (i < (static_cast<int>(cellIdsOriginalPlane->size()) - 1))
		{
			double nextCellId = cellIdsOriginalPlane->at(i + 1);
			double lastCellId = cellIdsOriginalPlane->at(i - 1);

			if ((currCellId + 1) != nextCellId)
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

vtkSmartPointer<vtkActor> iACompUniformTable::drawLine(
	double* startPoint, double* endPoint, double lineColor[3], double lineWidth)
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
	m_mainRenderer->AddActor(lineActor);

	return lineActor;
}

void iACompUniformTable::drawPointRepresentation()
{
	std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>*>::iterator it;
	int zoomedRowDataInd = m_zoomedRowData->size() - 1;

	//find max-min val of each dataset and same bin to enable comparison of the same bins in different datasets
	std::map<int, std::vector<double>*> minMaxPerBin = std::map<int, std::vector<double>*>();
	for (int datasetInd = 0; datasetInd < m_zoomedRowData->size(); datasetInd++)
	{
		for (int binId = 0; binId < static_cast<int>(m_zoomedRowData->at(datasetInd)->size()); binId++)
		{
			minMaxPerBin.insert({binId, new std::vector<double>()});

			for (int valId = 0; valId < static_cast<int>(m_zoomedRowData->at(datasetInd)->at(binId).size()); valId++)
			{
				auto binData = m_zoomedRowData->at(datasetInd)->at(binId);
				auto minMax = std::minmax_element(binData.begin(), binData.end());
				double currMin = *minMax.first;
				double currMax = *minMax.second;

				std::vector<double>* minMaxVec = minMaxPerBin.at(binId);
				if (minMaxVec->size() == 0)
				{  // if empty --> initialize
					minMaxVec->push_back(currMin);
					minMaxVec->push_back(currMax);
				}
				else
				{
					if (minMaxVec->at(0) > currMin)
					{
						minMaxVec->at(0) = currMin;
					}

					if (minMaxVec->at(1) < currMax)
					{
						minMaxVec->at(1) = currMax;
					}
				}
			}
		}
	}

	//draw point presentation
	for (int counter = 0; counter < m_vis->getAmountDatasets(); counter++)
	{
		int indData = m_vis->getOrderOfIndicesDatasets()->at(counter);

		it = originalPlaneZoomedPlanePair->find(m_originalPlaneActors->at(counter));

		if (it != originalPlaneZoomedPlanePair->end())
		{  //row
			vtkSmartPointer<vtkActor> originalRowAct = it->first;
			std::vector<vtkSmartPointer<vtkActor>>* zoomedRowActs = it->second;

			for (int zoomedRowInd = 0; zoomedRowInd < static_cast<int>(zoomedRowActs->size()); zoomedRowInd++)
			{
				vtkSmartPointer<vtkActor> zoomedRowAct = zoomedRowActs->at(zoomedRowInd);
				vtkSmartPointer<vtkAlgorithm> algorithm =
					zoomedRowAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
				vtkSmartPointer<vtkPolyDataAlgorithm> polyDataAlgorithm = vtkPolyDataAlgorithm::SafeDownCast(algorithm);
				vtkSmartPointer<vtkPolyData> currPolyData = polyDataAlgorithm->GetPolyDataInput(0);

				vtkSmartPointer<vtkDoubleArray> originArray =
					vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("originArray"));
				vtkSmartPointer<vtkDoubleArray> point1Array =
					vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point1Array"));
				vtkSmartPointer<vtkDoubleArray> point2Array =
					vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point2Array"));

				double origin[3];
				originArray->GetTuple(0, origin);

				double point1[3];
				point1Array->GetTuple(point1Array->GetNumberOfTuples() - 1, point1);

				double point2[3];
				point2Array->GetTuple(0, point2);

				double xmin = origin[0];
				double xmax = point1[0];
				double ymin = origin[1];
				double ymax = point2[1];

				auto iter = m_pickedCellsforPickedRow->find(indData);
				if (iter == m_pickedCellsforPickedRow->end())
				{
					continue;
				}

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
				iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK, col);
				planeActor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
				m_pointRepresentationActors->push_back(planeActor);
				m_mainRenderer->AddActor(planeActor);

				//set middle line
				double startP[3] = {xmin, ymin + ((ymax - ymin) / 2.0), 0.0};
				double endP[3] = {xmax, ymin + ((ymax - ymin) / 2.0), 0.0};
				double col1[3];
				iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col1);
				vtkSmartPointer<vtkActor> lineActor = drawLine(startP, endP, col1, 2);
				lineActor->GetProperty()->SetOpacity(0.1);
				lineActor->Modified();
				m_pointRepresentationActors->push_back(lineActor);

				//draw indiviudal data points
				double radius = m_vis->getColSize() * 0.10;
				double lineWidth = 1;
				double circleColor[3];
				iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, circleColor);
				double lineColor[3];
				iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, lineColor);

				std::vector<double> data = m_zoomedRowData->at(zoomedRowDataInd)->at(zoomedRowInd);

				double newY = ymin + ((ymax - ymin) / 2.0);
				//(newXMin + radius) --> so that min & max points do not lie on border
				vtkSmartPointer<vtkPoints> points = calculatePointPosition(
					data, (xmin + radius), (xmax - radius), newY);
				if (points == nullptr)
				{
					continue;
				}

				vtkSmartPointer<vtkActor> pointActor =
					drawPoints(points, circleColor, radius, lineColor, lineWidth);
				m_pointRepresentationActors->push_back(pointActor);
			}
			zoomedRowDataInd--;
		}
	}

	renderWidget();
}

vtkSmartPointer<vtkPolyData> iACompUniformTable::initializeRow(int numberOfBins)
{
	//row consisting of a certain number of bins

	vtkSmartPointer<vtkPoints> glyphPoints = vtkSmartPointer<vtkPoints>::New();
	glyphPoints->SetDataTypeToDouble();
	glyphPoints->SetNumberOfPoints(numberOfBins);

	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->SetPoints(glyphPoints);

	vtkSmartPointer<vtkDoubleArray> originArray = vtkSmartPointer<vtkDoubleArray>::New();
	originArray->SetName("originArray");
	originArray->SetNumberOfComponents(3);
	originArray->SetNumberOfTuples(numberOfBins);
	originArray->Modified();

	vtkSmartPointer<vtkDoubleArray> point1Array = vtkSmartPointer<vtkDoubleArray>::New();
	point1Array->SetName("point1Array");
	point1Array->SetNumberOfComponents(3);
	point1Array->SetNumberOfTuples(numberOfBins);
	point1Array->Modified();

	vtkSmartPointer<vtkDoubleArray> point2Array = vtkSmartPointer<vtkDoubleArray>::New();
	point2Array->SetName("point2Array");
	point2Array->SetNumberOfComponents(3);
	point2Array->SetNumberOfTuples(numberOfBins);
	point2Array->Modified();

	vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorArray->SetName("colorArray");
	colorArray->SetNumberOfComponents(3);
	colorArray->SetNumberOfTuples(numberOfBins);
	colorArray->Modified();

	polydata->GetPointData()->AddArray(originArray);
	polydata->GetPointData()->AddArray(point1Array);
	polydata->GetPointData()->AddArray(point2Array);
	polydata->GetCellData()->AddArray(colorArray);
	polydata->GetCellData()->SetActiveScalars("colorArray");
	polydata->Modified();

	return polydata;
}

vtkSmartPointer<vtkActor> iACompUniformTable::drawPoints(
	vtkSmartPointer<vtkPoints> points, double color[3], double radius, double lineColor[3], double lineWidth)
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

	m_mainRenderer->AddActor(actor);

	return actor;
}

vtkSmartPointer<vtkPoints> iACompUniformTable::calculatePointPosition(
	std::vector<double> dataPoints, double newMinX, double newMaxX, double y)
{
	/*LOG(lvlDebug,"dataPoints.size() = " + QString::number(dataPoints.size()));

	for (int i = 0; i < dataPoints.size(); i++)
	{
		LOG(lvlDebug,"Point " + QString::number(i) + ": " + QString::number(dataPoints.at(i)));
	}*/

	if (dataPoints.size() == 0)
	{  //when the list is empty
		return nullptr;
	}
	else if (dataPoints.size() == 1)
	{  //min and max are the same
		vtkSmartPointer<vtkPoints> result = vtkSmartPointer<vtkPoints>::New();
		result->InsertNextPoint(newMinX + ((newMaxX - newMinX) * 0.5), y, 0.0);
		return result;
	}

	vtkSmartPointer<vtkPoints> result = vtkSmartPointer<vtkPoints>::New();
	auto minMax = std::minmax_element(dataPoints.begin(), dataPoints.end());
	double min = *minMax.first;
	double max = *minMax.second;

	//use when point representation should be comparable
	/*double min = currMinMax.at(0);
	double max = currMinMax.at(1);*/

	for (int i = 0; i < static_cast<int>(dataPoints.size()); i++)
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

/******************************************  Interaction  **********************************************/
std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> iACompUniformTable::getSelectedData(
	Pick::PickedMap* map)
{
	//stores object attributes
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjects = new QList<std::vector<csvDataType::ArrayType*>*>();
	//stores MDS values
	QList<bin::BinType*>* thisZoomedRowData = new QList<bin::BinType*>();

	//get for all datasets each bin with its objects attributes (label,...)
	QList<std::vector<csvDataType::ArrayType*>*>* objectsPerBin = m_uniformBinningData->getObjectsPerBin();
	//get for all dataset each bin with its MDS values
	QList<bin::BinType*>* binData = m_uniformBinningData->getBinData();

	m_indexOfPickedRow = new std::vector<int>();
	m_pickedCellsforPickedRow = new std::map<int, std::vector<vtkIdType>*>();

	for (int rowId = m_vis->getAmountDatasets() - 1; rowId >= 0; rowId--)
	{
		vtkSmartPointer<vtkActor> currAct = m_originalPlaneActors->at(rowId);

		//set dataIndex
		std::map<vtkSmartPointer<vtkActor>, int>::iterator it = m_rowDataIndexPair->find(currAct);
		if (it == m_rowDataIndexPair->end())
			continue;
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
			for (int i = 0; i < static_cast<int>(pickedCells->size()); i++)
			{
				int currBin = pickedCells->at(i);
				newRowIds->push_back(currRowIds->at(currBin));
				newRowMDS->push_back(currRowMDS->at(currBin));
			}

			selectedObjects->append(newRowIds);
			thisZoomedRowData->append(newRowMDS);

			m_indexOfPickedRow->push_back(dataIndex);
			m_pickedCellsforPickedRow->insert({dataIndex, pickedCells});
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

void iACompUniformTable::highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId)
{
	vtkSmartPointer<vtkAlgorithm> currAlgorithm = pickedActor->GetMapper()->GetInputConnection(0, 0)->GetProducer();
	vtkSmartPointer<vtkPolyDataAlgorithm> polyDataAlgorithm = vtkPolyDataAlgorithm::SafeDownCast(currAlgorithm);
	vtkSmartPointer<vtkPolyData> currPolyData = polyDataAlgorithm->GetPolyDataInput(0);

	vtkSmartPointer<vtkDoubleArray> originArray =
		vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("originArray"));
	vtkSmartPointer<vtkDoubleArray> point1Array =
		vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point1Array"));
	vtkSmartPointer<vtkDoubleArray> point2Array =
		vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point2Array"));

	double origin[3];
	originArray->GetTuple(pickedCellId, origin);

	double point1[3];
	point1Array->GetTuple(pickedCellId, point1);

	double point2[3];
	point2Array->GetTuple(pickedCellId, point2);

	double xMax = point1[0];
	double yMax = point2[1];

	double rightUpperPoint[3] = {xMax, yMax, 0};

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	points->InsertNextPoint(origin);
	points->InsertNextPoint(point2);
	points->InsertNextPoint(rightUpperPoint);
	points->InsertNextPoint(point1);
	points->InsertNextPoint(origin);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);	
	
	vtkSmartPointer<vtkActor> selectedActor = drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);

	m_highlighingActors->push_back(selectedActor);
	m_mainRenderer->AddActor(selectedActor);

	renderWidget();
}

bool iACompUniformTable::removeHighlightedRow()
{
	if (m_mainRenderer->HasViewProp(m_highlightRowActor))
	{
		m_mainRenderer->RemoveActor(m_highlightRowActor);
		return true;
	}

	return false;
}

void iACompUniformTable::highlightSelectedRow(vtkSmartPointer<vtkActor> pickedActor)
{
	vtkSmartPointer<vtkAlgorithm> currAlgorithm = pickedActor->GetMapper()->GetInputConnection(0, 0)->GetProducer();
	vtkSmartPointer<vtkPolyDataAlgorithm> polyDataAlgorithm = vtkPolyDataAlgorithm::SafeDownCast(currAlgorithm);
	vtkSmartPointer<vtkPolyData> currPolyData = polyDataAlgorithm->GetPolyDataInput(0);

	vtkSmartPointer<vtkDoubleArray> originArray =
		vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("originArray"));
	vtkSmartPointer<vtkDoubleArray> point1Array =
		vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point1Array"));
	vtkSmartPointer<vtkDoubleArray> point2Array =
		vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point2Array"));

	double origin[3];
	originArray->GetTuple(0, origin);

	double point1[3];
	point1Array->GetTuple(point1Array->GetNumberOfTuples() - 1, point1);

	double point2[3];
	point2Array->GetTuple(0, point2);

	double xMax = point1[0];
	double yMax = point2[1];

	double rightUpperPoint[3] = { xMax , yMax, 0 };

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	points->InsertNextPoint(origin);
	points->InsertNextPoint(point2);
	points->InsertNextPoint(rightUpperPoint);
	points->InsertNextPoint(point1);
	points->InsertNextPoint(origin);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);

	m_highlightRowActor = drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);

	renderWidget();
}

double iACompUniformTable::calculateChiSquaredMetric(bin::BinType* observedFrequency, bin::BinType* expectedFrequency)
{
	double chiSquare = 0.0;

	for (int i = 0; i < static_cast<int>(observedFrequency->size()); i++)
	{//for each bin

		if (expectedFrequency->at(i).size() != 0)
		{
			int diff = ((int)observedFrequency->at(i).size()) - ((int)expectedFrequency->at(i).size());
			chiSquare += std::pow(((double)diff), 2) / ((double)expectedFrequency->at(i).size());
		}
		else
		{
			int diff = ((int)observedFrequency->at(i).size());
			chiSquare += std::pow(((double)diff), 2);
		}
	}

	return chiSquare;
}

void iACompUniformTable::reorderHistogramTable(vtkSmartPointer<vtkActor> movingActor)
{
	//calculate in which row the moving actor will be set
	m_newDrawingPosition = calculateCurrentPosition(movingActor);

	if (m_oldDrawingPosition != -1)
	{
		//reorder the indices that describe the drawing order
		reorderIndices(m_newDrawingPosition, m_oldDrawingPosition);
	}
}

int iACompUniformTable::calculateCurrentPosition(vtkSmartPointer<vtkActor> movingActor)
{
	double yPos = movingActor->GetCenter()[1];

	std::map<int, std::vector<double>>* drawingPositionForRegions = m_vis->getDrawingPositionForRegions();
	std::map<int, std::vector<double>>::iterator iter = drawingPositionForRegions->begin();

	double minPossibleY = INFINITY;
	double maxPossibleY = -INFINITY;

	//inside defined drawing area
	while (iter != drawingPositionForRegions->end())
	{
		std::vector<double> yInterval = iter->second;
		double yMin = yInterval.at(0);
		double yMax = yInterval.at(1);

		if (yPos >= yMin && yPos <= yMax)
		{
			return iter->first;
		}

		if (minPossibleY > yMin)
			minPossibleY = yMin;
		if (maxPossibleY < yMax)
			maxPossibleY = yMax;

		iter++;
	}

	//outside defined drawing area
	if (yPos < minPossibleY)
	{
		return 0;
	}

	if (yPos > maxPossibleY)
	{
		return m_vis->getAmountDatasets() - 1;
	}

	return -1;
}

void iACompUniformTable::reorderIndices(int newDrawingPos, int oldDrawingPos)
{
	//initialize storage for indices during moving of actor
	std::vector<int>* newOrderOfIndicesDatasets = m_vis->getNewOrderOfIndicesDatasets();
	iACompVisOptions::copyVector(m_vis->getOrderOfIndicesDatasets(), newOrderOfIndicesDatasets);

	int difference = newDrawingPos - oldDrawingPos;

	if (difference > 0)
	{  //reorder when difference is positive --> actor is moving up

		int currDataIndx = newOrderOfIndicesDatasets->at(newDrawingPos);
		newOrderOfIndicesDatasets->at(newDrawingPos) = newOrderOfIndicesDatasets->at(oldDrawingPos);

		for (int i = 1; i <= difference; i++)
		{
			int nextDataIndx = newOrderOfIndicesDatasets->at(newDrawingPos - i);
			newOrderOfIndicesDatasets->at(newDrawingPos - i) = currDataIndx;
			currDataIndx = nextDataIndx;
		}
	}
	else if (difference < 0)
	{  //reorder when difference is negative --> actor is moving down

		int currDataIndx = newOrderOfIndicesDatasets->at(newDrawingPos);
		newOrderOfIndicesDatasets->at(newDrawingPos) = newOrderOfIndicesDatasets->at(oldDrawingPos);

		for (int i = 1; i <= std::abs(difference); i++)
		{
			int nextDataIndx = newOrderOfIndicesDatasets->at(newDrawingPos + i);
			newOrderOfIndicesDatasets->at(newDrawingPos + i) = currDataIndx;
			currDataIndx = nextDataIndx;
		}
	}
}

void iACompUniformTable::removePointRepresentation()
{
	for (unsigned int i = 0; i < m_pointRepresentationActors->size(); i++)
	{
		m_mainRenderer->RemoveActor(m_pointRepresentationActors->at(i));
	}

	m_pointRepresentationActors->clear();
}

void iACompUniformTable::calculateOldDrawingPositionOfMovingActor(vtkSmartPointer<vtkActor> movingActor)
{
	//get oldPosition of the moving actor
	std::vector<vtkSmartPointer<vtkActor>>::iterator it =
		std::find(m_originalPlaneActors->begin(), m_originalPlaneActors->end(), movingActor);
	m_oldDrawingPosition = std::distance(m_originalPlaneActors->begin(), it);
}

void iACompUniformTable::removeBarCharShowingAmountOfObjects()
{
	m_useDarkerLut = false;

	for (int i = 0; i < static_cast<int>(m_barActors->size()); i++)
	{
		m_mainRenderer->RemoveActor(m_barActors->at(i));
		m_mainRenderer->RemoveActor2D(m_barTextActors->at(i));
	}

	m_barActors->clear();
	m_barTextActors->clear();
}

/******************************************  Recompute Binning  **********************************************/

void iACompUniformTable::recomputeBinning()
{
	m_uniformBinningData = (iACompUniformBinningData*) m_vis->recalculateBinning(iACompVisOptions::binningType::Uniform, m_bins);
}

/******************************************  Getter & Setter  **********************************************/
std::vector<vtkSmartPointer<vtkActor>>* iACompUniformTable::getOriginalRowActors()
{
	return m_originalPlaneActors;
}

vtkSmartPointer<vtkActor> iACompUniformTable::getHighlightingRowActor()
{
	return m_highlightRowActor;
}

int iACompUniformTable::getBins()
{
	return m_bins;
}

void iACompUniformTable::setBins(int bins)
{
	m_bins = bins;
}

int iACompUniformTable::getMinBins()
{
	return minBins;
}

int iACompUniformTable::getMaxBins()
{
	return maxBins;
}

int iACompUniformTable::getBinsZoomed()
{
	return m_binsZoomed;
}

void iACompUniformTable::setBinsZoomed(int bins)
{
	m_binsZoomed = bins;
}

vtkSmartPointer<iACompUniformTableInteractorStyle> iACompUniformTable::getInteractorStyle()
{
	return m_interactionStyle;
}

/******************************************  Ordering/Ranking  **********************************/

void iACompUniformTable::drawHistogramTableInOriginalOrder()
{
	std::vector<int>* originalOrderOfIndicesDatasets = m_vis->getOriginalOrderOfIndicesDatasets();
	std::vector<int>* orderOfIndicesDatasets = m_vis->getOrderOfIndicesDatasets();

	if (m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	iACompVisOptions::copyVector(originalOrderOfIndicesDatasets, orderOfIndicesDatasets);
	m_useDarkerLut = true;

	drawHistogramTable(m_bins);

	std::vector<int> amountObjectsEveryDataset = *(m_uniformBinningData->getAmountObjectsEveryDataset());
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
}

void iACompUniformTable::drawHistogramTableInDescendingOrder()
{
	std::vector<int> amountObjectsEveryDataset = *(m_uniformBinningData->getAmountObjectsEveryDataset());

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = m_vis->sortWithMemory(amountObjectsEveryDataset, 1);
	m_vis->setOrderOfIndicesDatasets(m_vis->reorderAccordingTo(newOrder));

	if (m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	m_useDarkerLut = true;

	drawHistogramTable(m_bins);
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
}

void iACompUniformTable::drawHistogramTableInAscendingOrder()
{
	std::vector<int> amountObjectsEveryDataset = *(m_uniformBinningData->getAmountObjectsEveryDataset());

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = m_vis->sortWithMemory(amountObjectsEveryDataset, 0);
	m_vis->setOrderOfIndicesDatasets(m_vis->reorderAccordingTo(newOrder));

	if (m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	m_useDarkerLut = true;

	drawHistogramTable(m_bins);
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
}

void iACompUniformTable::drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset)
{
	std::vector<int>* orderOfIndicesDatasets = m_vis->getOrderOfIndicesDatasets();

	//define bars
	auto minMax = std::minmax_element(begin(amountObjectsEveryDataset), end(amountObjectsEveryDataset));
	int max = *minMax.second;

	for (int i = 0; i < static_cast<int>(m_originalPlaneActors->size()); i++)
	{
		vtkSmartPointer<vtkActor> currAct = m_originalPlaneActors->at(i);
		vtkSmartPointer<vtkAlgorithm> currAlgorithm = currAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
		vtkSmartPointer<vtkPolyDataAlgorithm> polyDataAlgorithm = vtkPolyDataAlgorithm::SafeDownCast(currAlgorithm);
		vtkSmartPointer<vtkPolyData> currPolyData = polyDataAlgorithm->GetPolyDataInput(0);

		createBarChart(currPolyData, amountObjectsEveryDataset.at(orderOfIndicesDatasets->at(i)), max);
	}

	renderWidget();
}

/******************************************  Update THIS  **********************************/

void iACompUniformTable::showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType)
{
	//remove the old selection
	removeSelectionOfCorrelationMap();

	for (std::map<int, double>::iterator iter = dataIndxSelectedType->begin(); iter != dataIndxSelectedType->end();
		 iter++)
	{
		int selectedDataInd = iter->first;

		std::map<vtkSmartPointer<vtkActor>, int>::iterator iterActor;
		for (iterActor = m_rowDataIndexPair->begin(); iterActor != m_rowDataIndexPair->end(); iterActor++)
		{  //find actor according to dataIndex

			int dataInd = iterActor->second;

			if (dataInd == selectedDataInd)
			{
				vtkSmartPointer<vtkActor> selectedAct = iterActor->first;

				vtkSmartPointer<vtkAlgorithm> algorithm =
					selectedAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
				vtkSmartPointer<vtkPolyDataAlgorithm> polyDataAlgorithm = vtkPolyDataAlgorithm::SafeDownCast(algorithm);
				vtkSmartPointer<vtkPolyData> currPolyData = polyDataAlgorithm->GetPolyDataInput(0);

				vtkSmartPointer<vtkDoubleArray> originArray =
					vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("originArray"));
				vtkSmartPointer<vtkDoubleArray> point1Array =
					vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point1Array"));
				vtkSmartPointer<vtkDoubleArray> point2Array =
					vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point2Array"));

				if (iter->second == 0.0)
				{  // is outer arc --> highlight whole dataset

					double originPlane[3];
					originArray->GetTuple(0, originPlane);
					double point1Plane[3];
					point1Array->GetTuple(point1Array->GetNumberOfTuples() - 1, point1Plane);
					double point2Plane[3];

					point2Array->GetTuple(0, point2Plane);
					double col[3];
					iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);

					drawStippledTexture(originPlane, point1Plane, point2Plane, col);
				}
				else if (iter->second == 1.0)
				{  // is selected inner arc --> highlight selected cells

					std::map<int, std::vector<vtkIdType>*>::iterator pos =
						m_pickedCellsforPickedRow->find(selectedDataInd);
					if (pos != m_pickedCellsforPickedRow->end())
					{
						std::vector<vtkIdType>* cells = pos->second;

						for (int i = 0; i < static_cast<int>(cells->size()); i++)
						{
							vtkIdType cellId = cells->at(i);

							double origin[3];
							originArray->GetTuple(cellId, origin);
							double point1[3];
							point1Array->GetTuple(cellId, point1);
							double point2[3];
							point2Array->GetTuple(cellId, point2);

							double col[3];
							iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);
							drawStippledTexture(origin, point1, point2, col);
						}
					}
				}
				else
				{  //is not-selected inner arc --> highlight all other cells (not the selected ones)
					std::map<int, std::vector<vtkIdType>*>::iterator pos =
						m_pickedCellsforPickedRow->find(selectedDataInd);
					if (pos != m_pickedCellsforPickedRow->end())
					{
						std::vector<vtkIdType>* cells = pos->second;
						std::vector<vtkIdType>::iterator cellIterator;

						for (int i = 0; i < m_bins; i++)
						{
							double origin[3];
							originArray->GetTuple(i, origin);
							double point1[3];
							point1Array->GetTuple(i, point1);
							double point2[3];
							point2Array->GetTuple(i, point2);

							cellIterator = std::find(cells->begin(), cells->end(), i);
							if (cellIterator == cells->end())
							{  //when the cell is not one of the selected ones
								double col[3];
								iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTERGREY, col);
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

void iACompUniformTable::drawStippledTexture(double* origin, double* point1, double* point2, double* color)
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

	m_mainRenderer->AddActor(actor);
	m_stippledActors->push_back(actor);
}

void iACompUniformTable::removeSelectionOfCorrelationMap()
{
	for (int i = 0; i < static_cast<int>(m_stippledActors->size()); i++)
	{
		m_mainRenderer->RemoveActor(m_stippledActors->at(i));
	}

	m_stippledActors->clear();
}
