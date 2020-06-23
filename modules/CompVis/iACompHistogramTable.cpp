#include "iACompHistogramTable.h"

//Debug
#include "iAConsole.h"

//CompVis
#include "iACompVisOptions.h"
#include "iACompVisMain.h"

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

#include <vtkCamera.h>

#include <vtkDataSetMapper.h>
#include <vtkExtractSelection.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkTriangleFilter.h>
#include <vtkUnstructuredGrid.h>

#include <algorithm>
#include <cstring>
#include <functional>


iACompHistogramTable::iACompHistogramTable(
	MainWindow* parent, iAMultidimensionalScaling* mds, iACsvDataStorage* dataStorage, iACompVisMain* main) :
	QDockWidget(parent),
	m_main(main),
	m_mds(mds),
	m_inputData(mds->getCSVFileData()),
	m_dataStorage(dataStorage),
	m_BinRangeLength(0),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_renderer(vtkSmartPointer<vtkRenderer>::New()),
	m_tableSize(10),
	m_indexOfZoomedRows(new std::vector<int>()),
	m_pointRepresentationActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_datasetNameActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_highlighingActors(new std::vector<vtkSmartPointer<vtkActor>>())
{
	//initialize GUI
	setupUi(this);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	m_qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(m_qvtkWidget);

	std::vector<int>* dataResolution = csvFileData::getAmountObjectsEveryDataset(m_inputData);
	m_amountDatasets = dataResolution->size();

	m_bins = minBins;
	m_binsZoomed = minBins;

	//initialize datastructure
	calculateHistogramTable();

	//setup rendering environment
	m_renderer->SetBackground(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	m_renderer->SetViewport(0, 0, 0.8, 1);
	m_qvtkWidget->GetRenderWindow()->AddRenderer(m_renderer);
}

void iACompHistogramTable::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);
	
	if (m_qvtkWidget->height() > (m_qvtkWidget->width()*0.9))
	{
		screenRatio = ((double)m_qvtkWidget->width()*0.9) / ((double)m_qvtkWidget->height());
		m_colSize = 1;
		m_rowSize = screenRatio;
	}
	else 
	{
		screenRatio = ((double)m_qvtkWidget->height()) / ((double)m_qvtkWidget->width());
		m_colSize = (screenRatio / m_amountDatasets);
		m_rowSize = 1; 
	}

	//initialize visualization
	initializeHistogramTable();
}

/******************************************  Rendering  **********************************************/
void iACompHistogramTable::drawHistogramTable(int bins)
{
	m_colSize = (screenRatio / m_amountDatasets);

	if (m_renderer->GetViewProps()->GetNumberOfItems() > 0)
	{
		m_renderer->RemoveAllViewProps();
	}

	//draw cells from bottom to top --> so start with last dataset and go to first
	int currCol = 0;
	for (int dataInd = m_amountDatasets - 1; dataInd >= 0; dataInd--)
	{
		drawRow(dataInd, currCol, bins, 0);
		currCol += 1;
	}

	renderWidget();
}

void iACompHistogramTable::drawLinearZoom(Pick::PickedMap* map, int notSelectedBinNumber, int selectedBinNumber, QList<bin::BinType*>* zoomedRowData)
{
	QList<bin::BinType*>* thisZoomedRowData = bin::DeepCopy(zoomedRowData);
	//store bin data for zoom of selected rows
	zoomedRowData = bin::DeepCopy(thisZoomedRowData);

	//draw zoomed histogram table
	m_indexOfZoomedRows->clear();
	m_renderer->RemoveAllViewProps();

	int currCol = 0;
	double offset = 0;
	double distance = m_colSize * 0.2;
	bool addedRow = false;
	m_colSize = (screenRatio / (m_amountDatasets + map->size()));

	for (int indData = (m_amountDatasets - 1); indData >= 0; indData--)
	{
		//check for additional row
		std::vector<int>::iterator it = std::find(m_indexOfPickedData->begin(), m_indexOfPickedData->end(), indData);
		if (it != m_indexOfPickedData->end())
		{
			m_indexOfZoomedRows->push_back((m_amountDatasets + map->size() - 1) - currCol);
			offset = offset + distance;
			drawZoomedRow(indData, currCol, selectedBinNumber, thisZoomedRowData->last(), offset);
			thisZoomedRowData->removeLast();

			currCol += 1;
			addedRow = true;
		}

		//draw original datasets
		drawRow(indData, currCol, notSelectedBinNumber, offset);

		if (addedRow)
		{
			offset = offset + distance;
			addedRow = false;
		}
		currCol += 1;
	}

	renderWidget();
}

void iACompHistogramTable::drawRow(int currDataInd, int currentColumn, int amountOfBins, double offset)
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

	//DEBUG_LOG("drawRow ");
	//DEBUG_LOG("currentColumn = " + QString::number(currentColumn));
	//DEBUG_LOG("offset = " + QString::number(offset));
	//DEBUG_LOG("Origin = (" + QString::number(0.0) + ", " + QString::number(y) + ")");
	//DEBUG_LOG("Point1 = (" + QString::number(x) + ", " + QString::number(y) + ")");
	//DEBUG_LOG("Point2 = (" + QString::number(0.0) + ", " + QString::number(y + m_colSize) + ")");

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
	actor->GetProperty()->EdgeVisibilityOn();
	double col[3];
	col[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[0];
	col[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[1];
	col[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[2];
	actor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
	m_renderer->AddActor(actor);

	//add name of dataset/row
	double pos[3] = { -m_rowSize*0.05, y + (m_colSize*0.5), 0.0 };
	addDatasetName(currDataInd, pos);
}

void iACompHistogramTable::drawZoomedRow(
	int currDataInd, int currentColumn, int amountOfBins, bin::BinType* currentData, double offset)
{
	//##### TODO ####
	//repair non linear zooming --> recalculate whole table with ratio of height/width!!!!!

	int bins = currentData->size();
	int resultingAmountBins = bins * amountOfBins;

	vtkSmartPointer<vtkPlaneSource> aPlane = vtkSmartPointer<vtkPlaneSource>::New();
	aPlane->SetXResolution(resultingAmountBins);
	aPlane->SetYResolution(m_ColForData);

	double x = m_rowSize;
	double y = (m_colSize * currentColumn) + offset;
	aPlane->SetOrigin(0, y, 0.0);
	aPlane->SetPoint1(x, y, 0.0);               //width
	aPlane->SetPoint2(0, y + m_colSize, 0.0);  // height
	aPlane->Update();

	//DEBUG_LOG("drawZoomedRow ");
	//DEBUG_LOG("currentColumn = " + QString::number(currentColumn));
	//DEBUG_LOG("offset = " + QString::number(offset));
	//DEBUG_LOG("Origin = (" + QString::number(0.0) + ", " + QString::number(y) + ")");
	//DEBUG_LOG("Point1 = (" + QString::number(x) + ", " + QString::number(y) + ")");
	//DEBUG_LOG("Point2 = (" + QString::number(0.0) + ", " + QString::number(y + m_colSize) + ")");

	vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorData->SetName("colors");
	colorData->SetNumberOfComponents(3);

	colorRowForZoom(colorData, currentData, amountOfBins);
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
	col[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[0];
	col[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[1];
	col[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE)[2];
	actor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
	actor->GetProperty()->SetLineWidth(1);

	vtkSmartPointer<vtkNamedColors> nc = vtkSmartPointer<vtkNamedColors>::New();
	actor->GetProperty()->SetColor(nc->GetColor3d("DarkGray").GetData());
	m_renderer->AddActor2D(actor);
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

			colorRowForZoom(
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

	renderWidget();
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

	renderWidget();
}

void iACompHistogramTable::removePointRepresentation()
{
	for (unsigned int i = 0; i < m_pointRepresentationActors->size(); i++)
	{
		m_renderer->RemoveActor(m_pointRepresentationActors->at(i));
	}

	m_pointRepresentationActors->clear();
}

void iACompHistogramTable::renderWidget()
{
	m_qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
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

	//sequential coloring from red to yellow to blue
	QColor c1 = QColor(165, 0, 38);
	QColor c2 = QColor(215, 48, 39);
	QColor c3 = QColor(244, 109, 67);
	QColor c4 = QColor(253, 174, 97);
	QColor c5 = QColor(254, 224, 144);
	QColor c6 = QColor(224, 243, 248);
	QColor c7 = QColor(171, 217, 233);
	QColor c8 = QColor(116, 173, 209);
	QColor c9 = QColor(69, 117, 180);
	QColor c10 = QColor(49, 54, 149);

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

		//position description in the middle of each color bar in the scalarBar legend
		m_lut->SetAnnotation(low + ((high-low)*0.5), lowerString + " - " + upperString);
		
		//store min and max value of the dataset
		if (i == 0) 
		{
			min = low;
		}else if( i == m_tableSize-1)
		{
			max = high;
		}
	}

	m_lut->SetTableRange(min, max);
	m_lut->SetBelowRangeColor(0,0,0, 1);
	m_lut->UseBelowRangeColorOn();
}

void iACompHistogramTable::colorRow(vtkUnsignedCharArray* colors, int currDataset, int numberOfBins)
{
	QList<bin::BinType*>* binData = m_histData->calculateBins(numberOfBins);
	calculateBinLength();

	colorBinsOfRow(colors, binData->at(currDataset), binData->at(currDataset)->size());
}

void iACompHistogramTable::colorRowForZoom(vtkUnsignedCharArray* colors, bin::BinType* data, int amountOfBins)
{
	int bins = data->size();
	int resultingAmountBins = bins * amountOfBins;

	for (int currBin = 0; currBin < bins; currBin++)
	{  //for all selected bins in the original row

		bin::BinType* binData = m_histData->calculateBins(data, currBin, amountOfBins);

		if (binData == NULL || binData == nullptr)
		{
			for (int b = 0; b < amountOfBins; b++)
			{
				double rgb[3];
				m_lut->GetColor(-1, rgb);
				unsigned char* ucrgb = iACompVisOptions::getColorArray(rgb);
				colors->InsertNextTuple3(ucrgb[0], ucrgb[1], ucrgb[2]);
			}
			return;
		}

		colorBinsOfRow(colors, binData, binData->size());
	}
}

void iACompHistogramTable::colorBinsOfRow(vtkUnsignedCharArray* colors, bin::BinType* binData, int amountOfBins)
{
	for (int b = 0; b < amountOfBins; b++)
	{  //for each selected bin a specific amount of bins is drawn according to this data

		double rgb[3];
		int amountVals = binData->at(b).size();
		
		m_lut->GetColor(amountVals, rgb);

		unsigned char* ucrgb = iACompVisOptions::getColorArray(rgb);
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

	//initialize legend
	initializeLegend();

	//initialize interaction
	initializeInteraction();

	//draw histogramTable
	drawHistogramTable(m_bins);

	//render
	//m_renderer->ResetCamera();
	//init camera
	vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
	camera->SetPosition(m_rowSize*0.5, (m_colSize*m_amountDatasets)*0.5, 1);
	camera->SetFocalPoint( m_rowSize*0.5, (m_colSize*m_amountDatasets)*0.5, 0);
	//camera->SetParallelScale(screenRatio * m_amountDatasets);
	m_renderer->SetActiveCamera(camera);

	renderWidget();
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
	scalarBar->GetTitleTextProperty()->SetVerticalJustificationToTop();
	scalarBar->SetVerticalTitleSeparation(7);
	scalarBar->GetTitleTextProperty()->Modified();

	//text properties
	vtkSmartPointer<vtkTextProperty> propL = vtkSmartPointer<vtkTextProperty>::New();
	propL->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	propL->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TEXT));
	propL->Modified();
	scalarBar->SetAnnotationTextProperty(propL);

	// Setup render window, renderer, and interactor
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
	renderer->SetViewport(0.8, 0, 1, 1);
	renderer->SetBackground(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	renderer->AddActor2D(scalarBar);
	m_qvtkWidget->GetRenderWindow()->AddRenderer(renderer);
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
	legendProperty->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE));
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
	style->SetCurrentRenderer(m_renderer);
	style->setIACompVisMain(m_main);

	m_qvtkWidget->GetInteractor()->SetInteractorStyle(style);
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

double iACompHistogramTable::round_up(double value, int decimal_places)
{
	const double multiplier = std::pow(10.0, decimal_places);
	return std::ceil(value * multiplier) / multiplier;
}

/******************************************  Data Caculation  **********************************************/
void iACompHistogramTable::calculateHistogramTable()
{
	m_histData = new iACompHistogramTableData(m_mds);
}

/******************************************  Interaction  **********************************************/
QList<bin::BinType*>* iACompHistogramTable::getSelectedData(Pick::PickedMap* map)
{
	//TODO give correct data to main
	DEBUG_LOG("in selected data!");
	//first actor contains last dataset
	vtkSmartPointer<vtkActorCollection> actorList = m_renderer->GetActors();

	QList<bin::BinType*>* binData = m_histData->getBinData();
	QList<bin::BinType*>* thisZoomedRowData = new QList<bin::BinType*>();
	m_indexOfPickedData = new std::vector<int>();

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
			m_indexOfPickedData->push_back(abs((m_amountDatasets - 1) - dataIndex));
		}

		dataIndex--;
	}

	//DEBUG
	/*DEBUG_LOG("DEBUGGING");
	for (int i = 0; i < thisZoomedRowData->size(); i++)
	{
		bin::debugBinType(thisZoomedRowData->at(i));
	}*/

	return thisZoomedRowData;
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
	selectedActor->GetProperty()->SetEdgeColor(1, 0, 0);
	selectedActor->GetProperty()->SetLineWidth(2);

	m_highlighingActors->push_back(selectedActor);
	m_renderer->AddActor(selectedActor);

	renderWidget();
}

void iACompHistogramTable::removeHighlightedCells()
{
	for (int i = 0; i < m_highlighingActors->size(); i++)
	{
		m_renderer->RemoveActor(m_highlighingActors->at(i));
	}

	m_highlighingActors->clear();
}