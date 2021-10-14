#include "iACompUniformTable.h"

//CompVis
#include "iACompUniformBinningData.h"
#include "iACompHistogramVis.h"

//Qt
#include <QColor>

//vtk
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkColorTransferFunction.h"
#include "vtkLookupTable.h"
#include "vtkScalarBarActor.h"
#include "vtkTextProperty.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkTextActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkPlaneSource.h"
#include "vtkCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkCamera.h"

#include "vtkSelectionNode.h"
#include "vtkUnsignedCharArray.h"
#include "vtkSelection.h"
#include "vtkExtractSelection.h"
#include "vtkDataSetMapper.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyLine.h"
#include "vtkProp.h"
#include "vtkNamedColors.h"
#include "vtkLineSource.h"
#include "vtkSphereSource.h"
#include "vtkGlyph2D.h"
#include "vtkProperty2D.h"

#include <vtkBorderRepresentation.h>
#include <vtkBorderWidget.h>

iACompUniformTable::iACompUniformTable(iACompHistogramVis* vis, iACompUniformBinningData* uniformBinningData) :
	iACompTable(vis), 
	m_uniformBinningData(uniformBinningData),
	m_originalPlaneActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_zoomedPlaneActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_BinRangeLength(0),
	originalPlaneZoomedPlanePair(new std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>*>()),
	m_highlightRowActor(vtkSmartPointer<vtkActor>::New()),
	m_pointRepresentationActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_stippledActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_oldDrawingPosition(-1),
	m_newDrawingPosition(-1),
	m_interactionStyle(vtkSmartPointer<iACompUniformTableInteractorStyle>::New())
{
	m_bins = minBins;
	m_binsZoomed = minBins;

	//initialize interaction
	initializeInteraction();

	//initialize visualization
	initializeTable();
}

void iACompUniformTable::initializeTable()
{
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
		drawHistogramTable(m_bins);

		//init camera
		initializeCamera();
	}
	else if (m_lastState == iACompVisOptions::lastState::Defined)
	{
		m_interactionStyle->reinitializeState();

		drawHistogramTable(m_bins);
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
void iACompUniformTable::makeLUTFromCTF()
{
	calculateBinRange();

	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToRGB();

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
	int startVal = 1;
	
	for (size_t i = 0; i < m_tableSize; i++)
	{
		double* rgb;

		//make format of annotations
		double low = round_up(startVal + (i * m_BinRangeLength), 2);
		double high = round_up(startVal + ((i + 1) * m_BinRangeLength), 2);

		std::string sLow = std::to_string(low);
		std::string sHigh = std::to_string(high);

		std::string lowerString = initializeLegendLabels(sLow);
		std::string upperString = initializeLegendLabels(sHigh);

		//store min and max value of the dataset
		if (i == 0)
		{
			min = low;
		}
		else if (i == m_tableSize - 1)
		{
			max = high;
		}

		
		rgb = ctf->GetColor(static_cast<double>(i) / (double)m_tableSize);
		m_lut->SetTableValue(i, rgb);
		//position description in the middle of each color bar in the scalarBar legend
		m_lut->SetAnnotation(low + ((high - low) * 0.5), lowerString + " - " + upperString);
	}

	m_lut->SetTableRange(min, max);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	m_lut->SetBelowRangeColor(col[0], col[1], col[2], 1);
	m_lut->UseBelowRangeColorOn();
}

void iACompUniformTable::makeLUTDarker()
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
	int startVal = 1;

	for (size_t i = 0; i < m_tableSize; i++)
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
		m_lutDarker->SetAnnotation(low + ((high - low) * 0.5), lowerString + " - " + upperString);

		//store min and max value of the dataset
		if (i == 0)
		{
			min = low;
		}
		else if (i == m_tableSize - 1)
		{
			max = high;
		}
	}

	m_lutDarker->SetTableRange(min, max);

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	m_lutDarker->SetBelowRangeColor(col[0], col[1], col[2], 1);
	m_lutDarker->UseBelowRangeColorOn();
}

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
	}
	m_originalPlaneActors->clear();
	m_zoomedPlaneActors->clear();
	m_rowDataIndexPair->clear();

	//draw cells from bottom to top --> so start with last dataset and go to first
	for (int currCol = 0; currCol < m_vis->getAmountDatasets(); currCol++)
	{
		int dataInd = m_vis->getOrderOfIndicesDatasets()->at(currCol);
		drawRow(dataInd, currCol, bins, 0);
	}

	renderWidget();

	m_lastState = iACompVisOptions::lastState::Defined;
}

vtkSmartPointer<vtkPlaneSource> iACompUniformTable::drawRow(int currDataInd, int currentColumn, int amountOfBins, double offset)
{
	vtkSmartPointer<vtkPlaneSource> aPlane = vtkSmartPointer<vtkPlaneSource>::New();
	aPlane->SetXResolution(amountOfBins);
	aPlane->SetYResolution(m_ColForData);

	double x = m_vis->getRowSize();
	double y = (m_vis->getColSize() * currentColumn) + offset;
	aPlane->SetOrigin(0, y, 0.0);
	aPlane->SetPoint1(x, y, 0.0);               //width
	aPlane->SetPoint2(0, y + m_vis->getColSize(), 0.0);  // height
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

	if (!m_useDarkerLut)
	{ //the edges of the cells are drawn
		actor->GetProperty()->EdgeVisibilityOn();
		double col[3];
		iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_BLACK, col);
		actor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
		actor->GetProperty()->SetLineWidth(actor->GetProperty()->GetLineWidth()*1.5);
	}
	else
	{ //not showing the edges of the cells
		actor->GetProperty()->EdgeVisibilityOff();
	}

	m_mainRenderer->AddActor(actor);

	//store row and for each row the index which dataset it is showing
	m_originalPlaneActors->push_back(actor);
	m_rowDataIndexPair->insert({ actor, currDataInd });

	//add name of dataset/row
	double pos[3] = { -(m_vis->getRowSize()) * 0.05, y + (m_vis->getColSize()* 0.5), 0.0 };
	addDatasetName(currDataInd, pos);

	return aPlane;
}

void iACompUniformTable::colorRow(vtkUnsignedCharArray* colors, int currDataset, int numberOfBins)
{
	m_uniformBinningData = static_cast<iACompUniformBinningData*>(m_vis->recalculateBinning(iACompVisOptions::binningType::Uniform, numberOfBins));
	QList<bin::BinType*>* binData = m_uniformBinningData->getBinData();
	calculateBinRange();

	colorBinsOfRow(colors, binData->at(currDataset), ((int)binData->at(currDataset)->size()));
}

void iACompUniformTable::colorBinsOfRow(vtkUnsignedCharArray* colors, bin::BinType* binData, int amountOfBins)
{
	for (int b = 0; b < amountOfBins; b++)
	{  //for each selected bin a specific amount of bins is drawn according to this data

		double rgb[3];
		int amountVals = (int)binData->at(b).size();

		if (m_useDarkerLut)
		{
			m_lutDarker->GetColor(amountVals, rgb);
		}
		else
		{
			m_lut->GetColor(amountVals, rgb);
		}

		unsigned char ucrgb[3];
		iACompVisOptions::getColorArray3(rgb, ucrgb);
		colors->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
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
	for (int i = 0; i < indexOfCells->size(); i++)
	{
		highlightSelectedCell(m_originalPlaneActors->at(m_vis->getAmountDatasets() - 1), indexOfCells->at(i));
	}
}

void iACompUniformTable::drawLinearZoom(
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
	double newHeight = windowHeight - (((windowHeight * distance) * 4) * map->size()) -
		(((windowHeight * distanceToParent)) * map->size());
	m_vis->calculateRowWidthAndHeight(windowWidth, newHeight, m_vis->getAmountDatasets() + map->size());

	std::vector<vtkSmartPointer<vtkPlaneSource>>* zoomedPlanes = new std::vector<vtkSmartPointer<vtkPlaneSource>>();
	vtkSmartPointer<vtkPlaneSource> originalPlane;

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
			zoomedPlanes =
				drawZoomedRow(currCol, selectedBinNumber, thisZoomedRowData->last(), offset, cellIds);
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

				for (int i = 0; i < cellIds->size(); i++)
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
			std::vector<vtkSmartPointer<vtkActor>>* zoomedActorsForThisRow =
				new std::vector<vtkSmartPointer<vtkActor>>();
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
}

std::vector<vtkSmartPointer<vtkPlaneSource>>* iACompUniformTable::drawZoomedRow(int currentColumn,
	int amountOfBins, bin::BinType* currentData, double offsetHeight, std::vector<vtkIdType>* cellIdsOriginalPlane)
{
	std::vector<vtkSmartPointer<vtkPlaneSource>>* zoomedPlanes = new std::vector<vtkSmartPointer<vtkPlaneSource>>();

	double rowSize = m_vis->getRowSize();
	double colSize = m_vis->getColSize();

	double widthOriginal = rowSize + rowSize * 0.7;
	double widthRange = widthOriginal / currentData->size();
	double offsetWidth = rowSize * 0.05;

	double startX = -rowSize * 0.35;
	double endX = startX + widthRange;
	double startY = (colSize * currentColumn) + offsetHeight;
	double endY = startY + (1.5 * colSize);

	for (int i = 0; i < cellIdsOriginalPlane->size(); i++)
	{
		vtkSmartPointer<vtkPlaneSource> plane =
			drawZoomedPlanes(amountOfBins, startX, startY, endX, endY, i, currentData);

		zoomedPlanes->push_back(plane);

		if (i < cellIdsOriginalPlane->size() - 1)
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

void iACompUniformTable::redrawZoomedRow(int selectedBinNumber)
{
	int rowId = m_zoomedRowData->size() - 1;
	int cellId = 0;
	for (int zoomedRowDataInd = 0; zoomedRowDataInd < m_zoomedPlaneActors->size(); zoomedRowDataInd++)
	{  //for all zoomed planes

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
		if (cellId >= (m_zoomedRowData->at(rowId)->size() - 1))
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

void iACompUniformTable::colorRowForZoom(vtkUnsignedCharArray* colors, int currBin, bin::BinType* data, int amountOfBins)
{
	m_vis->calculateSpecificBins(iACompVisOptions::binningType::Uniform, data, currBin, amountOfBins);
	bin::BinType* binData = m_uniformBinningData->getZoomedBinData();

	if (binData == nullptr)
	{
		for (int b = 0; b < amountOfBins; b++)
		{
			double rgb[3];
			m_lut->GetColor(-1, rgb);
			unsigned char ucrgb[3];
			iACompVisOptions::getColorArray3(rgb, ucrgb);
			colors->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
		}
		return;
	}

	colorBinsOfRow(colors, binData, ((int)binData->size()));
}

vtkSmartPointer<vtkPlaneSource> iACompUniformTable::drawZoomedPlanes(
	int bins, double startX, double startY, double endX, double endY, int currBinIndex, bin::BinType* currentData)
{
	vtkSmartPointer<vtkPlaneSource> aPlane = vtkSmartPointer<vtkPlaneSource>::New();
	aPlane->SetXResolution(bins);
	aPlane->SetYResolution(m_ColForData);

	aPlane->SetOrigin(startX, startY, 0.0);
	aPlane->SetPoint1(endX, startY, 0.0);  //width
	aPlane->SetPoint2(startX, endY, 0.0);  // height
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
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	actor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
	actor->GetProperty()->SetLineWidth(1);

	vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
	actor->GetProperty()->SetColor(nc->GetColor3d("DarkGray").GetData());
	m_mainRenderer->AddActor2D(actor);

	m_zoomedPlaneActors->push_back(actor);

	return aPlane;
}

void iACompUniformTable::drawLineBetweenRowAndZoomedRow(std::vector<vtkSmartPointer<vtkPlaneSource>>* zoomedRowPlanes,
	vtkSmartPointer<vtkPlaneSource> originalRowPlane, std::vector<vtkIdType>* cellIdsOriginalPlane)
{
	int amountOfCells = originalRowPlane->GetXResolution();
	double xMinO = originalRowPlane->GetOrigin()[0];
	double yMinO = originalRowPlane->GetOrigin()[1];
	double xMaxO = originalRowPlane->GetPoint1()[0];
	double widthO = xMaxO - xMinO;
	double binLengthO = widthO / ((double)amountOfCells);

	for (int i = 0; i < zoomedRowPlanes->size(); i++)
	{
		vtkSmartPointer<vtkPlaneSource> zoomedRowPlane = zoomedRowPlanes->at(i);
		double xMinZ = zoomedRowPlane->GetOrigin()[0];
		double xMaxZ = zoomedRowPlane->GetPoint1()[0];
		double yMinZ = zoomedRowPlane->GetOrigin()[1];
		double yMaxZ = zoomedRowPlane->GetPoint2()[1];

		double currCellId = cellIdsOriginalPlane->at(i);

		double col[3];
		iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);

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
		else if (i == (cellIdsOriginalPlane->size() - 1))
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
		else if (i < cellIdsOriginalPlane->size() - 1)
		{
			//double currCellId = cellIdsOriginalPlane->at(i);
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
		for (int binId = 0; binId < m_zoomedRowData->at(datasetInd)->size(); binId++)
		{
			minMaxPerBin.insert({binId, new std::vector<double>()});

			for (int valId = 0; valId < m_zoomedRowData->at(datasetInd)->at(binId).size(); valId++)
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

			vtkSmartPointer<vtkAlgorithm> algorithm1 =
				originalRowAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
			vtkSmartPointer<vtkPlaneSource> originalPlane = vtkPlaneSource::SafeDownCast(algorithm1);

			for (int zoomedRowInd = 0; zoomedRowInd < zoomedRowActs->size(); zoomedRowInd++)
			{
				vtkSmartPointer<vtkActor> zoomedRowAct = zoomedRowActs->at(zoomedRowInd);
				vtkSmartPointer<vtkAlgorithm> algorithm =
					zoomedRowAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
				vtkSmartPointer<vtkPlaneSource> zoomedPlane = vtkPlaneSource::SafeDownCast(algorithm);

				double xmin = zoomedPlane->GetOrigin()[0];
				double xmax = zoomedPlane->GetPoint1()[0];
				double ymin = zoomedPlane->GetOrigin()[1];
				double ymax = zoomedPlane->GetPoint2()[1];

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
				double radius = (m_vis->getColSize() * 0.5) * 0.25;
				double lineWidth = 1;
				double circleColor[3];
				iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, circleColor);
				double lineColor[3];
				iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, lineColor);

				std::vector<double> data = m_zoomedRowData->at(zoomedRowDataInd)->at(zoomedRowInd);

				double newY = ymin + ((ymax - ymin) / 2.0);
				//(newXMin + radius) --> so that min & max points do not lie on border
				vtkSmartPointer<vtkPoints> points = calculatePointPosition(
					data, (xmin + radius), (xmax - radius), newY, *minMaxPerBin.at(zoomedRowInd));
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
	std::vector<double> dataPoints, double newMinX, double newMaxX, double y, std::vector<double> currMinMax)
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

	for (int i = 0; i < dataPoints.size(); i++)
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
			for (int i = 0; i < pickedCells->size(); i++)
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
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);

	selectedActor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
	selectedActor->GetProperty()->SetLineWidth(iACompVisOptions::LINE_WIDTH);

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
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);

	m_highlightRowActor = drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);

	renderWidget();
}

double iACompUniformTable::calculateChiSquaredMetric(bin::BinType* observedFrequency, bin::BinType* expectedFrequency)
{
	double chiSquare = 0.0;

	for (int i = 0; i < observedFrequency->size(); i++)
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

	for (int i = 0; i < m_barActors->size(); i++)
	{
		m_mainRenderer->RemoveActor(m_barActors->at(i));
		m_mainRenderer->RemoveActor2D(m_barTextActors->at(i));
	}

	m_barActors->clear();
	m_barTextActors->clear();
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

const int iACompUniformTable::getMinBins()
{
	return minBins;
}

const int iACompUniformTable::getMaxBins()
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

//std::vector<int>* iACompUniformTable::getIndexOfPickedRows()
//{
//	return m_indexOfPickedRow;
//}

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

	for (int i = 0; i < m_originalPlaneActors->size(); i++)
	{
		vtkSmartPointer<vtkActor> currAct = m_originalPlaneActors->at(i);
		vtkSmartPointer<vtkAlgorithm> currAlgorithm = currAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
		vtkSmartPointer<vtkPlaneSource> currPlane = vtkPlaneSource::SafeDownCast(currAlgorithm);

		createBarChart(currPlane, amountObjectsEveryDataset.at(orderOfIndicesDatasets->at(i)), max);
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
				vtkSmartPointer<vtkPlaneSource> oldPlane = vtkPlaneSource::SafeDownCast(algorithm);

				if (iter->second == 0.0)
				{  // is outer arc --> highlight whole dataset

					double col[3];
					iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_LIGHTGREY, col);

					drawStippledTexture(oldPlane->GetOrigin(), oldPlane->GetPoint1(), oldPlane->GetPoint2(), col);
				}
				else if (iter->second == 1.0)
				{  // is selected inner arc --> highlight selected cells

					std::map<int, std::vector<vtkIdType>*>::iterator pos =
						m_pickedCellsforPickedRow->find(selectedDataInd);
					if (pos != m_pickedCellsforPickedRow->end())
					{
						std::vector<vtkIdType>* cells = pos->second;

						double startX = oldPlane->GetOrigin()[0];
						double cellWidth = (oldPlane->GetPoint1()[0] - oldPlane->GetOrigin()[0]) / m_bins;

						for (int i = 0; i < cells->size(); i++)
						{
							vtkIdType cellId = cells->at(i);

							double startXCell = startX + (cellWidth * cellId);
							double endXCell = startXCell + cellWidth;

							double origin[3] = {startXCell, oldPlane->GetOrigin()[1], oldPlane->GetOrigin()[2]};
							double point1[3] = {endXCell, oldPlane->GetPoint1()[1], oldPlane->GetPoint1()[2]};
							double point2[3] = {startXCell, oldPlane->GetPoint2()[1], oldPlane->GetPoint2()[2]};

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

						double startX = oldPlane->GetOrigin()[0];
						double cellWidth = (oldPlane->GetPoint1()[0] - oldPlane->GetOrigin()[0]) / m_bins;

						for (int i = 0; i < m_bins; i++)
						{
							cellIterator = std::find(cells->begin(), cells->end(), i);
							if (cellIterator == cells->end())
							{  //when the cell is not one of the selected ones
								double startXCell = startX + (cellWidth * i);
								double endXCell = startXCell + cellWidth;

								double origin[3] = {startXCell, oldPlane->GetOrigin()[1], oldPlane->GetOrigin()[2]};
								double point1[3] = {endXCell, oldPlane->GetPoint1()[1], oldPlane->GetPoint1()[2]};
								double point2[3] = {startXCell, oldPlane->GetPoint2()[1], oldPlane->GetPoint2()[2]};

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
	for (int i = 0; i < m_stippledActors->size(); i++)
	{
		m_mainRenderer->RemoveActor(m_stippledActors->at(i));
	}

	m_stippledActors->clear();
}