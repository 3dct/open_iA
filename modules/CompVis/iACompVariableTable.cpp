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
	m_interactionStyle(vtkSmartPointer<iACompVariableTableInteractorStyle>::New()),
	m_bbData(bayesianBlocksData), 
	m_nbData(naturalBreaksData),
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

void iACompVariableTable::makeLUTFromCTF()
{
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

	double binRange = calculateUniformBinRange();

	for (size_t i = 0; i < m_tableSize; i++)
	{
		double* rgb;
		rgb = ctf->GetColor(static_cast<double>(i) / (double)m_tableSize);
		m_lut->SetTableValue(i, rgb);

		//make format of annotations
		double low = round_up(startVal + (i * binRange), 2);
		double high = round_up(startVal + ((i + 1) * binRange), 2);

		std::string sLow = std::to_string(low);
		std::string sHigh = std::to_string(high);

		std::string lowerString = initializeLegendLabels(sLow);
		std::string upperString = initializeLegendLabels(sHigh);

		//position description in the middle of each color bar in the scalarBar legend
		//m_lut->SetAnnotation(low + ((high - low) * 0.5), lowerString + " - " + upperString);
		m_lut->SetAnnotation(low + ((high - low) * 0.5), lowerString);

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

	m_lut->SetTableRange(min, max);

	/*double rgbMin[3];
	m_lut->GetColor(min, rgbMin);
	double rgbMax[3];
	m_lut->GetColor(max, rgbMax);
	LOG(lvlDebug,
		" LUT min = " + QString::number(min) + " with rgb = " + QString::number(rgbMin[0] * 255) + ", " +
			QString::number(rgbMin[1] * 255) + ", " + QString::number(rgbMin[2] * 255));
	LOG(lvlDebug,
		" LUT max = " + QString::number(max) + " with rgb = " + QString::number(rgbMax[0] * 255) + ", " +
			QString::number(rgbMax[1] * 255) + ", " + QString::number(rgbMax[2] * 255));
	*/

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	m_lut->SetBelowRangeColor(col[0], col[1], col[2], 1);
	m_lut->UseBelowRangeColorOn();

	double* colAbove = ctf->GetColor(1);
	m_lut->SetAboveRangeColor(colAbove[0], colAbove[1], colAbove[2], 1);
	m_lut->UseAboveRangeColorOn();
}

void iACompVariableTable::makeLUTDarker()
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

	double binRange = calculateUniformBinRange();

	for (size_t i = 0; i < m_tableSize; i++)
	{
		double* rgb;
		rgb = ctf->GetColor(static_cast<double>(i) / (double)m_tableSize);
		m_lutDarker->SetTableValue(i, rgb);

		//make format of annotations
		double low = round_up(startVal + (i * binRange), 2);
		double high = round_up(startVal + ((i + 1) * binRange), 2);

		std::string sLow = std::to_string(low);
		std::string sHigh = std::to_string(high);

		std::string lowerString = initializeLegendLabels(sLow);
		std::string upperString = initializeLegendLabels(sHigh);

		//position description in the middle of each color bar in the scalarBar legend
		//m_lut->SetAnnotation(low + ((high - low) * 0.5), lowerString + " - " + upperString);
		m_lutDarker->SetAnnotation(low + ((high - low) * 0.5), lowerString);

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

	/*double rgbMin[3];
	m_lut->GetColor(min, rgbMin);
	double rgbMax[3];
	m_lut->GetColor(max, rgbMax);
	LOG(lvlDebug,
		" LUT min = " + QString::number(min) + " with rgb = " + QString::number(rgbMin[0] * 255) + ", " +
			QString::number(rgbMin[1] * 255) + ", " + QString::number(rgbMin[2] * 255));
	LOG(lvlDebug,
		" LUT max = " + QString::number(max) + " with rgb = " + QString::number(rgbMax[0] * 255) + ", " +
			QString::number(rgbMax[1] * 255) + ", " + QString::number(rgbMax[2] * 255));
	*/

	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	m_lutDarker->SetBelowRangeColor(col[0], col[1], col[2], 1);
	m_lutDarker->UseBelowRangeColorOn();

	double* colAbove = ctf->GetColor(1);
	m_lutDarker->SetAboveRangeColor(colAbove[0], colAbove[1], colAbove[2], 1);
	m_lutDarker->UseAboveRangeColorOn();
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

	for (int i = 0; i < m_originalRowActors->size(); i++)
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
void iACompVariableTable::showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType)
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