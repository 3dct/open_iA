#include "iACompVariableTable.h"

//CompVis
#include "iACompHistogramVis.h"
#include "iACompHistogramTableData.h"

//vtk
#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarBarActor.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkLookupTable.h"
#include "vtkActor.h"
#include "vtkColorTransferFunction.h"

#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkCellData.h"
#include "vtkProperty.h"
#include "vtkMapper.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAlgorithm.h"
#include "vtkDoubleArray.h"
#include "vtkProgrammableGlyphFilter.h"

#include "vtkSelectionNode.h"
#include "vtkSelection.h"
#include "vtkExtractSelection.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSetMapper.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkPlaneSource.h"

iACompVariableTable::iACompVariableTable(iACompHistogramVis* vis, iACompBayesianBlocksData* bayesianBlocksData, iACompNaturalBreaksData* naturalBreaksData):
	iACompTable(vis),
	m_bbData(bayesianBlocksData), 
	m_nbData(naturalBreaksData),
	m_interactionStyle(vtkSmartPointer<iACompVariableTableInteractorStyle>::New()),
	m_originalRowActors(new std::vector<vtkSmartPointer<vtkActor>>())
{
	m_activeData = m_nbData;
	
	//initialize interaction
	initializeInteraction();

	//initialize visualization
	initializeTable();
}

void iACompVariableTable::setActive()
{
	//add rendererColorLegend to widget
	addLegendRendererToWidget();

	if (m_lastState == iACompVisOptions::lastState::Undefined)
	{
		drawHistogramTable();

		//init camera
		m_mainRenderer->SetActiveCamera(m_vis->getCamera());
		renderWidget();

		m_lastState = iACompVisOptions::lastState::Defined;
	}
	else if (m_lastState == iACompVisOptions::lastState::Defined)
	{
		reinitalizeState();

		drawHistogramTable();
		renderWidget();
	}
	else if (m_lastState == iACompVisOptions::lastState::Changed)
	{
		drawHistogramTable();
		renderWidget();
	}
}

void iACompVariableTable::setInactive()
{
	m_mainRenderer->RemoveAllViewProps();
}

void iACompVariableTable::setActiveBinning(iACompVisOptions::binningType binningType)
{
	if (binningType == iACompVisOptions::binningType::BayesianBlocks)
	{
		m_activeData = m_bbData;
	}
	else if (binningType == iACompVisOptions::binningType::JenksNaturalBreaks)
	{
		m_activeData = m_nbData;
	}
	else
	{
		m_activeData = m_bbData;
	}
}

void iACompVariableTable::initializeCamera()
{
	m_mainRenderer->SetActiveCamera(m_vis->getCamera());
}

/****************************************** Initialization **********************************************/
void iACompVariableTable::initializeTable()
{
	m_BinRangeLength = calculateUniformBinRange();

	//setup color table
	makeLUTFromCTF();
	makeLUTDarker();

	//initialize legend
	initializeLegend();

	//init camera
	initializeCamera();	
}

void iACompVariableTable::initializeInteraction()
{
	m_interactionStyle->setVariableTableVisualization(this);
	m_interactionStyle->SetDefaultRenderer(m_mainRenderer);
	m_interactionStyle->setIACompHistogramVis(m_vis);
}

double iACompVariableTable::calculateUniformBinRange()
{
	int maxAmountInAllBins = m_activeData->getMaxAmountInAllBins();
	return ((double)maxAmountInAllBins) / ((double)m_tableSize);
}

void iACompVariableTable::calculateBinRange()
{
}

/****************************************** Getter & Setter **********************************************/
vtkSmartPointer<iACompVariableTableInteractorStyle> iACompVariableTable::getInteractorStyle()
{
	return m_interactionStyle;
}

std::vector<vtkSmartPointer<vtkActor>>* iACompVariableTable::getOriginalRowActors()
{
	return m_originalRowActors;
}

/****************************************** Rendering **********************************************/
void iACompVariableTable::drawHistogramTable()
{
	m_originalRowActors->clear();
	m_rowDataIndexPair->clear();

	if (m_mainRenderer->GetViewProps()->GetNumberOfItems() > 0)
	{
		m_mainRenderer->RemoveAllViewProps();
		m_activeData->resetBinPolyData();
	}

	m_vis->calculateRowWidthAndHeight(m_vis->getWindowWidth(), m_vis->getWindowHeight(), m_vis->getAmountDatasets());

	//draw cells from bottom to top --> so start with last dataset and go to first
	for (int currCol = 0; currCol < m_vis->getAmountDatasets(); currCol++)
	{
		int dataInd = m_vis->getOrderOfIndicesDatasets()->at(currCol);
		drawRow(dataInd, currCol, 0);
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

void iACompVariableTable::drawRow(int currDataInd, int currentColumn, double offset)
{
	bin::BinType* currDataset = m_activeData->getBinData()->at(currDataInd);
	double numberOfBins = currDataset->size();

	//each row consists of a certain number of bins and each bin will be drawn as glyph
	vtkSmartPointer<vtkPoints> glyphPoints = vtkSmartPointer<vtkPoints>::New();
	glyphPoints->SetDataTypeToDouble();
	glyphPoints->SetNumberOfPoints(numberOfBins);

	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->SetPoints(glyphPoints);

	vtkSmartPointer<vtkDoubleArray> originArray = vtkSmartPointer<vtkDoubleArray>::New();
	originArray->SetName("originArray");
	originArray->SetNumberOfComponents(3);
	originArray->SetNumberOfTuples(numberOfBins);
	
	vtkSmartPointer<vtkDoubleArray> point1Array = vtkSmartPointer<vtkDoubleArray>::New();
	point1Array->SetName("point1Array");
	point1Array->SetNumberOfComponents(3);
	point1Array->SetNumberOfTuples(numberOfBins);

	vtkSmartPointer<vtkDoubleArray> point2Array = vtkSmartPointer<vtkDoubleArray>::New();
	point2Array->SetName("point2Array");
	point2Array->SetNumberOfComponents(3);
	point2Array->SetNumberOfTuples(numberOfBins);

	vtkSmartPointer<vtkUnsignedCharArray> colorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorArray->SetName("colorArray");
	colorArray->SetNumberOfComponents(3);
	colorArray->SetNumberOfTuples(numberOfBins);

	this->constructBins(
		m_activeData, currDataset, originArray, point1Array, point2Array, colorArray, currDataInd, currentColumn, offset);

	polydata->GetPointData()->AddArray(originArray);
	polydata->GetPointData()->AddArray(point1Array);
	polydata->GetPointData()->AddArray(point2Array);
	polydata->GetCellData()->AddArray(colorArray); 
	polydata->GetCellData()->SetActiveScalars("colorArray");

	vtkSmartPointer<vtkPlaneSource> planeSource = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
	planeSource->SetCenter(0, 0, 0);
	planeSource->Update();

	vtkSmartPointer<vtkProgrammableGlyphFilter> glypher = vtkSmartPointer<vtkProgrammableGlyphFilter>::New();
	glypher->SetInputData(polydata);
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

	m_originalRowActors->push_back(actor);
	//store row and for each row the index which dataset it is showing
	m_rowDataIndexPair->insert({actor, currDataInd});
	
	//add name of dataset/row
	double y = (m_vis->getColSize() * currentColumn) + offset;
	double pos[3] = {-(m_vis->getRowSize()) * 0.05, y + (m_vis->getColSize() * 0.5), 0.0};
	addDatasetName(currDataInd, pos);

	//add X Ticks
	if (m_vis->getXAxis())
	{
		//drawing positions
		double min_x = 0.0;
		double max_x = m_vis->getRowSize();
		double min_y = (m_vis->getColSize() * currentColumn) + offset;
		double max_y = min_y + m_vis->getColSize();

		double drawingDimensions[4] = {min_x, max_x, min_y, max_y};
		double yheight = min_y;
		double tickLength = (max_y - min_y) * 0.05;
		drawXTicks(drawingDimensions, yheight, tickLength);
	}
}

void iACompVariableTable::reinitalizeState()
{
	m_useDarkerLut = false;
}

/****************************************** Ordering/Ranking **********************************************/

void iACompVariableTable::drawHistogramTableInAscendingOrder()
{
	std::vector<int> amountObjectsEveryDataset = *(m_activeData->getAmountObjectsEveryDataset());

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = m_vis->sortWithMemory(amountObjectsEveryDataset, 0);
	m_vis->setOrderOfIndicesDatasets(m_vis->reorderAccordingTo(newOrder));

	if (m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	m_useDarkerLut = true;

	drawHistogramTable();
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
	renderWidget();
}

void iACompVariableTable::drawHistogramTableInDescendingOrder()
{
	std::vector<int> amountObjectsEveryDataset = *(m_activeData->getAmountObjectsEveryDataset());

	//stores the new order of the datasets as their new position
	std::vector<int>* newOrder = m_vis->sortWithMemory(amountObjectsEveryDataset, 1);
	m_vis->setOrderOfIndicesDatasets(m_vis->reorderAccordingTo(newOrder));

	if (m_useDarkerLut)
	{
		removeBarCharShowingAmountOfObjects();
	}

	m_useDarkerLut = true;

	drawHistogramTable();
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
	renderWidget();
}

void iACompVariableTable::drawHistogramTableInOriginalOrder()
{
	std::vector<int>* originalOrderOfIndicesDatasets = m_vis->getOriginalOrderOfIndicesDatasets();
	std::vector<int>* orderOfIndicesDatasets = m_vis->getOrderOfIndicesDatasets();

	iACompVisOptions::copyVector(originalOrderOfIndicesDatasets, orderOfIndicesDatasets);
	m_useDarkerLut = true;

	drawHistogramTable();
	
	std::vector<int> amountObjectsEveryDataset = *(m_activeData->getAmountObjectsEveryDataset());
	drawBarChartShowingAmountOfObjects(amountObjectsEveryDataset);
	
	renderWidget();
}

void iACompVariableTable::drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset)
{
	std::vector<int>* orderOfIndicesDatasets = m_vis->getOrderOfIndicesDatasets();

	//define bars
	auto minMax = std::minmax_element(begin(amountObjectsEveryDataset), end(amountObjectsEveryDataset));
	int max = *minMax.second;

	for (int i = 0; i < static_cast<int>(m_originalRowActors->size()); i++)
	{
		vtkSmartPointer<vtkActor> currAct = m_originalRowActors->at(i);
		vtkSmartPointer<vtkPolyData> polyData = vtkProgrammableGlyphFilter::SafeDownCast(
			currAct->GetMapper()->GetInputConnection(0, 0)->GetProducer())
			->GetPolyDataInput(0);
		
		vtkSmartPointer<vtkDoubleArray> originArray = vtkDoubleArray::SafeDownCast(polyData->GetPointData()->GetArray("originArray"));
		vtkSmartPointer<vtkDoubleArray> point1Array = vtkDoubleArray::SafeDownCast(polyData->GetPointData()->GetArray("point1Array"));
		vtkSmartPointer<vtkDoubleArray> point2Array = vtkDoubleArray::SafeDownCast(polyData->GetPointData()->GetArray("point2Array"));

		double positions[4] = {
			originArray->GetTuple3(0)[0], point1Array->GetTuple3(point1Array->GetNumberOfTuples()-1)[0],  //x_min, x_max
			originArray->GetTuple3(0)[1], point2Array->GetTuple3(point2Array->GetNumberOfTuples() - 1)[1],  //y_min, y_max
		};

		createBarChart(positions, amountObjectsEveryDataset.at(orderOfIndicesDatasets->at(i)), max);
	}
}

/****************************************** Update THIS **********************************************/
void iACompVariableTable::showSelectionOfCorrelationMap(std::map<int, double>*)
{
}

void iACompVariableTable::removeSelectionOfCorrelationMap()
{
}

/****************************************** Interaction Picking **********************************************/
void iACompVariableTable::highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId)
{
	vtkSmartPointer<vtkPolyData> polyData = vtkProgrammableGlyphFilter::SafeDownCast(pickedActor->GetMapper()->GetInputConnection(0, 0)->GetProducer())->GetPolyDataInput(0);

	vtkSmartPointer<vtkPlaneSource> planeSource = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource->SetOrigin(polyData->GetPointData()->GetArray("originArray")->GetTuple3(pickedCellId));
	planeSource->SetPoint1(polyData->GetPointData()->GetArray("point1Array")->GetTuple3(pickedCellId));
	planeSource->SetPoint2(polyData->GetPointData()->GetArray("point2Array")->GetTuple3(pickedCellId));
	planeSource->Update();
	
	vtkSmartPointer<vtkUnsignedCharArray> selectedColorArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
	selectedColorArray->SetName("selectedColorArray");
	selectedColorArray->SetNumberOfComponents(3);
	selectedColorArray->SetNumberOfTuples(1);
	double* color = polyData->GetCellData()->GetArray("colorArray")->GetTuple3(pickedCellId);
	selectedColorArray->InsertTuple3(0, color[0], color[1], color[2]);

	vtkSmartPointer<vtkPolyDataMapper> selectedMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	selectedMapper->SetInputData(planeSource->GetOutput());
	selectedMapper->GetInput()->GetCellData()->SetScalars(selectedColorArray);
	selectedMapper->SetColorModeToDefault();
	selectedMapper->SetScalarModeToUseCellData();
	selectedMapper->ScalarVisibilityOn();

	vtkSmartPointer<vtkActor> selectedActor = vtkSmartPointer<vtkActor>::New();
	selectedActor->SetMapper(selectedMapper);
	selectedActor->GetProperty()->EdgeVisibilityOn();
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN, col);
	selectedActor->GetProperty()->SetEdgeColor(col);
	selectedActor->GetProperty()->SetLineWidth(iACompVisOptions::LINE_WIDTH);

	m_highlighingActors->push_back(selectedActor);
	m_mainRenderer->AddActor(selectedActor);
}

std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> iACompVariableTable::getSelectedData(
	Pick::PickedMap* map)
{
	//stores object attributes
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjects = new QList<std::vector<csvDataType::ArrayType*>*>();
	//stores MDS values
	QList<bin::BinType*>* thisZoomedRowData = new QList<bin::BinType*>();


	//TODO implement
	//get for all datasets each bin with its objects attributes (label,...)
	QList<std::vector<csvDataType::ArrayType*>*>* objectsPerBin = m_activeData->getObjectsPerBin();
	//get for all dataset each bin with its MDS values
	QList<bin::BinType*>* binData = m_activeData->getBinData();

	m_indexOfPickedRow = new std::vector<int>();
	m_pickedCellsforPickedRow = new std::map<int, std::vector<vtkIdType>*>();

	for (int rowId = m_vis->getAmountDatasets() - 1; rowId >= 0; rowId--)
	{
		vtkSmartPointer<vtkActor> currAct = m_originalRowActors->at(rowId);

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