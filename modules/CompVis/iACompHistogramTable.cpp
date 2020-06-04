#include "iACompHistogramTable.h"

//Debug
#include "iAConsole.h"

//CompVis


//iA
#include "mainwindow.h"

//Qt
#include <QColor>
#include "QVTKOpenGLNativeWidget.h"

//vtk
#include <vtkActor.h>
#include <vtkGenericOpenGLRenderWindow.h>
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

#include <vtkColorSeries.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataObject.h>
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

#include <vtkBillboardTextActor3D.h>
#include <vtkCaptionActor2D.h>
#include <vtkFollower.h>
#include <vtkTextActor.h>
#include <vtkTextRepresentation.h>
#include <vtkTextWidget.h>
#include <vtkVectorText.h>

#include <algorithm>
#include <cstring>
#include <functional>


iACompHistogramTable::iACompHistogramTable(
	MainWindow* parent, iAMultidimensionalScaling* mds, iACsvDataStorage* dataStorage) :
	QDockWidget(parent),
	m_mds(mds),
	m_inputData(mds->getCSVFileData()),
	m_dataStorage(dataStorage),
	m_BinRangeLength(0),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_renderer(vtkSmartPointer<vtkRenderer>::New()),
	m_tableSize(10),
	m_indexOfZoomedRows(new std::vector<int>()),
	m_pointRepresentationActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_datasetNameActors(new std::vector<vtkSmartPointer<vtkActor>>())
{
	//initialize GUI
	setupUi(this);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(qvtkWidget);

	std::vector<int>* dataResolution = csvFileData::getAmountObjectsEveryDataset(m_inputData);
	m_amountDatasets = dataResolution->size();

	m_bins = minBins;
	m_binsZoomed = minBins;

	//initialize datastructure
	calculateHistogramTable();
}

void iACompHistogramTable::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);

	m_qvtkWidgetHeight = qvtkWidget->height();
	m_qvtkWidgetWidth = qvtkWidget->width();
	m_colSize = ((double)m_qvtkWidgetHeight) / ((double)m_amountDatasets);
	m_rowSize = m_qvtkWidgetWidth;

	//initialize visualization
	initializeHistogramTable();
}

void iACompHistogramTable::calculateHistogramTable()
{
	m_histData = new iACompHistogramTableData(m_mds);
}

void iACompHistogramTable::makeLUTFromCTF()
{
	calculateCellRange();

	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToRGB();

	QColor red = QColor(164, 0, 0, 255);
	QColor white = QColor(255, 255, 255, 255);
	// red to white
	ctf->AddRGBPoint(1.0, red.redF(), red.greenF(), red.blueF());
	ctf->AddRGBPoint(0.9, red.redF(), red.greenF(), red.blueF());
	ctf->AddRGBPoint(0.0, white.redF(), white.greenF(), white.blueF());

	m_lut->SetNumberOfTableValues(m_tableSize);
	m_lut->Build();
	int startVal = 0;

	for (size_t i = 0; i < m_tableSize; i++)
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

		m_lut->SetAnnotation(static_cast<double>(i) / (double)m_tableSize, lowerString + " - " + upperString);
	}

	m_lut->Build();
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

void iACompHistogramTable::makeCellData(vtkUnsignedCharArray* colors, int currDataset, int numberOfBins)
{
	QList<bin::BinType*>* binData = m_histData->calculateBins(numberOfBins);
	calculateCellRange();

	calculateCellData(colors, binData->at(currDataset), binData->at(currDataset)->size());
}

void iACompHistogramTable::makeCellDataForZoom(vtkUnsignedCharArray* colors, bin::BinType* data, int amountOfBins)
{
	int bins = data->size();
	int resultingAmountBins = bins * amountOfBins;

	for (int currBin = 0; currBin < bins; currBin++)
	{  //for all selected bins in the original row

		bin::BinType* binData = m_histData->calculateBins(data, currBin, amountOfBins);
		//bin::debugBinType(binData);

		if (binData == NULL || binData == nullptr)
		{
			unsigned char ucrgb[3];
			double rgb[3];
			for (int b = 0; b < amountOfBins; b++)
			{
				m_lut->GetColor(static_cast<double>(0) / ((double)m_tableSize - 1), rgb);

				for (size_t j = 0; j < 3; ++j)
				{
					ucrgb[j] = static_cast<unsigned char>(rgb[j] * 255);
				}
				colors->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
			}

			return;
		}

		calculateCellData(colors, binData, binData->size());
	}
}

void iACompHistogramTable::calculateCellData(vtkUnsignedCharArray* colors, bin::BinType* binData, int amountOfBins)
{
	for (int b = 0; b < amountOfBins; b++)
	{  //for each selected bin a specific amount of bins is drawn according to this data

		double rgb[3];
		unsigned char ucrgb[3];

		int amountVals = binData->at(b).size();

		int startVal = 0;

		for (int i = 0; i < m_tableSize; i++)
		{
			double low = startVal + (i * m_BinRangeLength);
			double high = startVal + ((i + 1) * m_BinRangeLength);

			bool inside = checkCellRange((double)amountVals, low, high);
			if (i == m_tableSize - 1)
			{
				inside = ((double)amountVals == high);
			}
			if (inside)
			{
				m_lut->GetColor(static_cast<double>(i) / ((double)m_tableSize - 1), rgb);
				break;
			}
		}

		for (size_t j = 0; j < 3; ++j)
		{
			ucrgb[j] = static_cast<unsigned char>(rgb[j] * 255);
		}
		colors->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
	}
}

void iACompHistogramTable::calculateCellRange()
{
	int maxAmountInAllBins = m_histData->getMaxAmountInAllBins();
	m_BinRangeLength = ((double)maxAmountInAllBins) / ((double)m_tableSize);
}

bool iACompHistogramTable::checkCellRange(double value, double low, double high)
{
	return (low <= value) && (value < high);
}

void iACompHistogramTable::initializeHistogramTable()
{
	//setup color table
	makeLUTFromCTF();

	vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
	m_renderer->SetBackground(nc->GetColor3d(backgroundColor).GetData());
	m_renderer->SetViewport(0, 0, 0.9, 1);

	vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
	camera->SetPosition(0, 0, 20);
	camera->SetFocalPoint(0, 0, 0);
	m_renderer->SetActiveCamera(camera);

	vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	qvtkWidget->SetRenderWindow(renderWindow);
	qvtkWidget->GetRenderWindow()->AddRenderer(m_renderer);

	//initialize legend
	initializeLegend(renderWindow);

	//initialize interaction
	initializeInteraction();

	//draw histogramTable
	drawHistogramTable(m_bins);

	m_renderer->ResetCamera();
	m_renderer->GetActiveCamera()->Zoom(1.0);
	m_renderer->GetActiveCamera()->ParallelProjectionOn();
	m_renderer->GetActiveCamera()->Modified();

	qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
}

void iACompHistogramTable::drawHistogramTable(int bins)
{
	DEBUG_LOG(" ");
	DEBUG_LOG(" First ");
	DEBUG_LOG("m_qvtkWidgetHeight: " + QString::number(m_qvtkWidgetHeight));
	DEBUG_LOG("m_qvtkWidgetWidth: " + QString::number(m_qvtkWidgetWidth));
	DEBUG_LOG("qvtkWidget->height(): " + QString::number(qvtkWidget->height()));
	DEBUG_LOG("qvtkWidget->width(): " + QString::number(qvtkWidget->width()));

	m_colSize = m_qvtkWidgetHeight / m_amountDatasets;

	if (m_renderer->GetViewProps()->GetNumberOfItems() > 0)
	{
		m_renderer->RemoveAllViewProps();
	}

	//draw cells from bottom to top --> so start with last dataset and go to first
	for (int cols = m_amountDatasets - 1; cols >= 0; cols--)
	{
		drawRow(cols, cols, bins, 0);
	}

	qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
}

void iACompHistogramTable::drawRow(int currDataInd, int currentColumn, int amountOfBins, double offset)
{
	vtkSmartPointer<vtkPlaneSource> aPlane = vtkSmartPointer<vtkPlaneSource>::New();
	aPlane->SetXResolution(amountOfBins);
	aPlane->SetYResolution(m_ColForData);

	double x = m_rowSize;
	double y = (-(m_colSize * (currentColumn + 1) + (m_colSize)) + (offset));
	aPlane->SetOrigin(-x, y, 0.0);
	aPlane->SetPoint1(x, y, 0.0);               //width
	aPlane->SetPoint2(-x, y + m_colSize, 0.0);  // height
	aPlane->Update();

	//DEBUG_LOG("drawRow");
	//DEBUG_LOG("origin: " + QString::number(-x) + " " + QString::number(y));
	//DEBUG_LOG("width: " + QString::number(x) + " " + QString::number(y));
	//DEBUG_LOG("heigth: " + QString::number(-x) + " " + QString::number(y + (m_colSize)));
	//DEBUG_LOG("distance in heigth: " + QString::number((y + (m_colSize)) - y));

	vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorData->SetName("colors");
	colorData->SetNumberOfComponents(3);
	makeCellData(colorData, currDataInd, amountOfBins);
	aPlane->GetOutput()->GetCellData()->SetScalars(colorData);

	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(aPlane->GetOutputPort());
	mapper->SetScalarModeToUseCellData();
	mapper->Update();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	actor->GetProperty()->EdgeVisibilityOn();

	addDatasetName(currDataInd, aPlane->GetOrigin());

	m_renderer->AddActor(actor);
}

void iACompHistogramTable::drawRowZoomedRow(
	int currentColumn, int amountOfBins, bin::BinType* currentData, double offset)
{
	int bins = currentData->size();
	int resultingAmountBins = bins * amountOfBins;

	vtkSmartPointer<vtkPlaneSource> aPlane = vtkSmartPointer<vtkPlaneSource>::New();
	aPlane->SetXResolution(resultingAmountBins);
	aPlane->SetYResolution(m_ColForData);

	double x = m_rowSize;
	double y = (-(m_colSize * (currentColumn + 1) + (m_colSize)) + (offset));
	aPlane->SetOrigin(-x, y, 0.0);
	aPlane->SetPoint1(x, y, 0.0);               //width
	aPlane->SetPoint2(-x, y + m_colSize, 0.0);  // height
	aPlane->Update();

	vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorData->SetName("colors");
	colorData->SetNumberOfComponents(3);

	makeCellDataForZoom(colorData, currentData, amountOfBins);
	aPlane->GetOutput()->GetCellData()->SetScalars(colorData);

	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(aPlane->GetOutputPort());
	mapper->SetScalarModeToUseCellData();
	mapper->Update();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	actor->GetProperty()->EdgeVisibilityOn();
	actor->GetProperty()->SetLineWidth(3);

	vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
	actor->GetProperty()->SetColor(nc->GetColor3d("DarkGray").GetData());
	m_renderer->AddActor(actor);
}

void iACompHistogramTable::drawLinearZoom(Pick::PickedMap* map, int notSelectedBinNumber, int selectedBinNumber)
{
	//first actor contains last dataset
	vtkSmartPointer<vtkActorCollection> actorList = m_renderer->GetActors();
	m_renderer->RemoveAllViewProps();

	QList<bin::BinType*>* binData = m_histData->getBinData();
	QList<bin::BinType*>* thisZoomedRowData = new QList<bin::BinType*>();
	std::vector<int>* dataInd = new std::vector<int>();

	int initialNumberOfActors = actorList->GetNumberOfItems();
	int dataIndex = m_amountDatasets - 1;
	for (int currDataInd = (actorList->GetNumberOfItems() - 1); currDataInd >= 0; currDataInd--)
	{
		vtkSmartPointer<vtkActor> currAct = actorList->GetLastActor();
		actorList->RemoveItem(currDataInd);

		//when a zoomedRow is next --> ignore it
		//zoomed
		std::vector<int>::iterator iter =
			std::find(m_indexOfZoomedRows->begin(), m_indexOfZoomedRows->end(), currDataInd);
		if (iter != m_indexOfZoomedRows->end())
		{
			continue;
		}

		//for every row --> for every actor
		if (map->find(currAct) != map->end())
		{
			std::vector<vtkIdType>* pickedCells = map->find(currAct)->second;
			std::sort(pickedCells->begin(), pickedCells->end(), std::less<long long>());

			bin::BinType* currRow = binData->at((binData->size() - 1) - dataIndex);
			bin::BinType* newRow = new bin::BinType();

			//look for the selected cells in the current row
			for (int i = 0; i < pickedCells->size(); i++)
			{
				int currBin = pickedCells->at(i);
				newRow->push_back(currRow->at(currBin));
			}

			thisZoomedRowData->append(newRow);
			dataInd->push_back(abs((m_amountDatasets - 1) - dataIndex));
		}

		dataIndex--;
	}

	//store bin data for zoom of selected rows
	zoomedRowData = bin::DeepCopy(thisZoomedRowData);

	//draw zoomed histogram table
	m_indexOfZoomedRows->clear();

	m_colSize = m_qvtkWidgetHeight / (m_amountDatasets + map->size());

	int currCol = m_amountDatasets + map->size() - 1;
	double offset = 0;
	double distance = m_colSize * 0.2;
	bool addedRow = false;

	for (int indData = (m_amountDatasets - 1); indData >= 0; indData--)
	{
		//check for additional row
		std::vector<int>::iterator it = std::find(dataInd->begin(), dataInd->end(), indData);
		if (it != dataInd->end())
		{
			m_indexOfZoomedRows->push_back((m_amountDatasets + map->size() - 1) - currCol);
			offset = offset + distance;
			drawRowZoomedRow(currCol, selectedBinNumber, thisZoomedRowData->last(), offset);
			thisZoomedRowData->removeLast();

			currCol -= 1;
			addedRow = true;
		}

		//draw original datasets
		drawRow(indData, currCol, notSelectedBinNumber, offset);

		if (addedRow)
		{
			offset = offset + distance;
			addedRow = false;
		}
		currCol -= 1;
	}

	qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
}

void iACompHistogramTable::redrawZoomedRow(int selectedBinNumber)
{
	vtkSmartPointer<vtkActorCollection> actorList = m_renderer->GetActors();
	actorList->InitTraversal();
	int curSelectedActorId = 0;
	for (int currDataInd = 0; currDataInd <= (actorList->GetNumberOfItems() - 1); currDataInd++)
	{
		std::vector<int>::iterator iter =
			std::find(m_indexOfZoomedRows->begin(), m_indexOfZoomedRows->end(), currDataInd);
		vtkSmartPointer<vtkActor> currAct = actorList->GetNextActor();
		if (iter != m_indexOfZoomedRows->end())
		{
			vtkSmartPointer<vtkAlgorithm> algorithm = currAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
			vtkSmartPointer<vtkPlaneSource> plane = vtkPlaneSource::SafeDownCast(algorithm);

			int bins = zoomedRowData->at((zoomedRowData->size() - 1) - curSelectedActorId)->size();
			int resultingAmountBins = bins * selectedBinNumber;

			plane->SetXResolution(resultingAmountBins);
			plane->Update();

			vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
			colorData->SetName("colors");
			colorData->SetNumberOfComponents(3);

			makeCellDataForZoom(
				colorData, zoomedRowData->at((zoomedRowData->size() - 1) - curSelectedActorId), selectedBinNumber);

			plane->GetOutput()->GetCellData()->SetScalars(colorData);

			// Setup actor and mapper
			vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
			mapper->SetInputConnection(plane->GetOutputPort());
			mapper->SetScalarModeToUseCellData();
			mapper->Update();
			currAct->Modified();

			curSelectedActorId++;
		}
	}

	qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
}

void iACompHistogramTable::drawPointRepresentation()
{
	DEBUG_LOG("drawPointRepresentation ");

	vtkSmartPointer<vtkActorCollection> actorList = m_renderer->GetActors();
	actorList->InitTraversal();

	int curSelectedActorId = 0;
	for (int currDataInd = 0; currDataInd <= (actorList->GetNumberOfItems() - 1); currDataInd++)
	{
		std::vector<int>::iterator iter =
			std::find(m_indexOfZoomedRows->begin(), m_indexOfZoomedRows->end(), currDataInd);
		vtkSmartPointer<vtkActor> currAct = actorList->GetNextActor();
		if (iter != m_indexOfZoomedRows->end())
		{
			vtkSmartPointer<vtkAlgorithm> algorithm = currAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
			vtkSmartPointer<vtkPlaneSource> oldPlane = vtkPlaneSource::SafeDownCast(algorithm);
			double xmin = oldPlane->GetOrigin()[0];
			double xmax = oldPlane->GetPoint1()[0];
			double ymin = oldPlane->GetOrigin()[1];
			double ymax = oldPlane->GetPoint2()[1];

			//set plane
			vtkSmartPointer<vtkPlaneSource> pointPlane = vtkSmartPointer<vtkPlaneSource>::New();
			pointPlane->SetXResolution(1);
			pointPlane->SetYResolution(1);

			pointPlane->SetOrigin(oldPlane->GetOrigin());
			pointPlane->SetPoint1(oldPlane->GetPoint1());
			pointPlane->SetPoint2(oldPlane->GetPoint2());
			pointPlane->Update();

			vtkSmartPointer<vtkPolyDataMapper> planeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
			planeMapper->SetInputConnection(pointPlane->GetOutputPort());
			planeMapper->SetScalarModeToUseCellData();
			planeMapper->Update();

			vtkSmartPointer<vtkActor> planeActor = vtkSmartPointer<vtkActor>::New();
			planeActor->SetMapper(planeMapper);
			m_pointRepresentationActors->push_back(planeActor);

			//set black line
			double startP[3] = {xmin, ymin + ((ymax - ymin) / 2.0), 0.0};
			double endP[3] = {xmax, ymin + ((ymax - ymin) / 2.0), 0.0};
			vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
			lineSource->SetPoint1(startP);
			lineSource->SetPoint2(endP);
			lineSource->Update();

			vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
			lineMapper->SetInputConnection(lineSource->GetOutputPort());
			vtkSmartPointer<vtkActor> lineActor = vtkSmartPointer<vtkActor>::New();
			lineActor->SetMapper(lineMapper);
			lineActor->GetProperty()->SetLineWidth(4);
			lineActor->GetProperty()->SetColor(0, 0, 1);
			m_pointRepresentationActors->push_back(lineActor);

			//#### TODO DRAW POINTS with VertexGlyphFilter ####
			//get data -> histogram data and according to points get coordinates for drawing
		}
	}

	for (unsigned int i = 0; i < m_pointRepresentationActors->size(); i++)
	{
		m_renderer->AddActor(m_pointRepresentationActors->at(i));
	}

	qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
}

void iACompHistogramTable::removePointRepresentation()
{
	for (unsigned int i = 0; i < m_pointRepresentationActors->size(); i++)
	{
		m_renderer->RemoveActor(m_pointRepresentationActors->at(i));
	}

	m_pointRepresentationActors->clear();
}

void iACompHistogramTable::initializeLegend(vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow)
{
	vtkSmartPointer<vtkScalarBarActor> scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
	scalarBar->SetLookupTable(m_lut);
	scalarBar->SetHeight(0.85);
	scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	scalarBar->GetPositionCoordinate()->SetValue(0.001, 0.1, 0.0);
	scalarBar->SetWidth(0.5);
	scalarBar->SetUnconstrainedFontSize(1);

	scalarBar->SetTitle("        Amount of\n      Objects");
	scalarBar->SetNumberOfLabels(0);
	scalarBar->SetTextPositionToPrecedeScalarBar();

	//title properties
	scalarBar->GetTitleTextProperty()->BoldOn();
	scalarBar->GetTitleTextProperty()->SetFontSize(20);
	scalarBar->GetTitleTextProperty()->SetVerticalJustificationToTop();
	scalarBar->SetVerticalTitleSeparation(7);
	scalarBar->GetTitleTextProperty()->Modified();

	//text properties
	vtkSmartPointer<vtkTextProperty> propL = vtkSmartPointer<vtkTextProperty>::New();
	propL->SetFontSize(15);
	propL->SetColor(1.0, 1.0, 1.0);
	propL->Modified();
	scalarBar->SetAnnotationTextProperty(propL);

	// Setup render window, renderer, and interactor
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
	renderer->SetViewport(0.9, 0, 1, 1);
	vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
	renderer->SetBackground(nc->GetColor3d(backgroundColor).GetData());
	renderer->AddActor2D(scalarBar);
	renderWindow->AddRenderer(renderer);
}

void iACompHistogramTable::addDatasetName(int currDataset, double* position)
{
	//TODO draw names of the dataset left of table row
	//call method where needed
	//set position of text
	QStringList* filenames = m_dataStorage->getDatasetNames();
	std::string name = filenames->at(currDataset).toLocal8Bit().constData();
	name.erase(0, name.find_last_of("/\\") + 1);

	vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
	textSource->SetText(name.c_str());
	textSource->Update();

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(textSource->GetOutputPort());
	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	actor->GetProperty()->SetColor(1.0, 1.0, 1.0);

	double x = (1 / m_qvtkWidgetWidth) * (std::abs(position[0]) / 100.0);
	double y = ((std::abs(position[1]) / m_qvtkWidgetHeight) * 100.0) * (m_qvtkWidgetHeight / 100.0);
	double newPos[3] = {(position[0] - (m_qvtkWidgetWidth*0.5)), (position[1] + (m_colSize*0.5)), position[2]};
	//double newPos[3] = {x, y, position[2]};

	DEBUG_LOG("position: " + QString::number(position[0]) + " " + QString::number(position[1]) + " " +
			  QString::number(position[2]) + " ");
	DEBUG_LOG("newPos: " + QString::number(newPos[0]) + " " + QString::number(newPos[1]) + " " +
			  QString::number(newPos[2]) + " ");

	/*vtkSmartPointer<vtkTextActor> textActor = vtkSmartPointer<vtkTextActor>::New();
	textActor->SetInput(name.c_str());
	//textActor->SetPosition(10, 40);

	actor->SetPosition(newPos);
	actor->Modified();

	textActor->SetPosition(0, newPos[1]);
	textActor->SetPosition2(70, newPos[1] + 40);
	//textActor->SetTextScaleModeToProp();

	textActor->GetTextProperty()->SetFontSize(12);
	textActor->GetTextProperty()->SetColor(1.0, 1.0, 1.0);

	//actor->SetScale((m_qvtkWidgetWidth*0.01));
	textActor->Modified();*/

	  vtkSmartPointer<vtkCoordinate> coordinate = vtkSmartPointer<vtkCoordinate>::New();
	coordinate->SetCoordinateSystemToWorld();
	  coordinate->SetValue(newPos);
	int* display = coordinate->GetComputedDisplayValue(m_renderer);

	DEBUG_LOG("display: " + QString::number(display[0]) + " " + QString::number(display[1]));

	/*vtkSmartPointer<vtkCaptionActor2D> captionActor = vtkSmartPointer<vtkCaptionActor2D>::New();
	captionActor->SetCaption(name.c_str());
	//captionActor->SetAttachmentPoint(newPos);
	captionActor->SetPosition(display[0], display[1]);
	captionActor->SetPosition2(display[0] + 70, display[1] + 40);
	//captionActor->BorderOff();
	captionActor->GetCaptionTextProperty()->BoldOff();
	captionActor->GetCaptionTextProperty()->ItalicOff();
	captionActor->GetCaptionTextProperty()->ShadowOff();
	captionActor->ThreeDimensionalLeaderOff();

	m_renderer->AddViewProp(captionActor);*/


	//m_renderer->AddActor2D(textActor);
}

void iACompHistogramTable::initializeInteraction()
{
	vtkSmartPointer<iACompHistogramTableInteractorStyle> style =
		vtkSmartPointer<iACompHistogramTableInteractorStyle>::New();
	style->setIACompHistogramTable(this);
	style->SetDefaultRenderer(m_renderer);
	style->SetCurrentRenderer(m_renderer);

	qvtkWidget->GetInteractor()->SetInteractorStyle(style);
}

vtkSmartPointer<vtkActor> iACompHistogramTable::createOutline(vtkDataObject* object, double color[3])
{
	vtkSmartPointer<vtkOutlineFilter> outline = vtkSmartPointer<vtkOutlineFilter>::New();
	outline->SetInputData(object);
	vtkSmartPointer<vtkPolyDataMapper> outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	outlineMapper->SetInputConnection(outline->GetOutputPort());
	vtkSmartPointer<vtkActor> outlineActor = vtkSmartPointer<vtkActor>::New();
	outlineActor->SetMapper(outlineMapper);
	outlineActor->GetProperty()->SetColor(color[0], color[1], color[2]);

	return outlineActor;
}

double iACompHistogramTable::round_up(double value, int decimal_places)
{
	const double multiplier = std::pow(10.0, decimal_places);
	return std::ceil(value * multiplier) / multiplier;
}

int iACompHistogramTable::getBins()
{
	return m_bins;
}

void iACompHistogramTable::setBins(int bins)
{
	m_bins = bins;
}

const int iACompHistogramTable::getMinBins()
{
	return minBins;
}

const int iACompHistogramTable::getMaxBins()
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

void iACompHistogramTable::clearZoomedRows()
{
	m_indexOfZoomedRows->clear();
}

vtkSmartPointer<vtkRenderer> iACompHistogramTable::getRenderer()
{
	return m_renderer;
}

void iACompHistogramTable::renderWidget()
{
	qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
}