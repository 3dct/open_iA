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
#include <functional>
#include <vector>


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
	m_pointRepresentationActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_datasetNameActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_highlighingActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_originalPlaneActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_zoomedPlaneActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	originalPlaneZoomedPlanePair(new std::map<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkActor>>())
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
	m_renderer->SetUseFXAA(true);
	m_qvtkWidget->GetRenderWindow()->AddRenderer(m_renderer);

}

void iACompHistogramTable::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);
	
	m_windowWidth = (double)m_qvtkWidget->width();
	m_windowHeight = (double)m_qvtkWidget->height();

	calculateRowWidthAndHeight(m_windowWidth, m_windowHeight, m_amountDatasets);

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

	//draw zoomed histogram table new
	m_renderer->RemoveAllViewProps();
	m_originalPlaneActors->clear();
	m_zoomedPlaneActors->clear();
	originalPlaneZoomedPlanePair->clear();

	int currCol = 0;
	double offset = 0;
	double distance = 0.03; //distance to other rows in percent [0,1]
	double distanceToParent = 0.015; //distance to row from which it is the zoom representation in percent [0,1]
	bool addedRow = false;
	bool offsetAlreadyAdded = false;

	double newHeight = m_windowHeight - (((m_windowHeight*distance) * 4)*map->size()) - (((m_windowHeight*distanceToParent))*map->size());
	calculateRowWidthAndHeight(m_windowWidth, newHeight, m_amountDatasets+map->size());

	vtkSmartPointer<vtkPlaneSource> zoomedPlane;
	vtkSmartPointer<vtkPlaneSource> originalPlane;

	for (int indData = (m_amountDatasets - 1); indData >= 0; indData--)
	{
		//check for additional row
		std::vector<int>::iterator it = std::find(m_indexOfPickedRow->begin(), m_indexOfPickedRow->end(), indData);
		if (it != m_indexOfPickedRow->end())
		{
			if (currCol != 0 && !offsetAlreadyAdded) 
			{//do not add a offset when the zoomed row is the undermost
				offset = offset + distance;
			}
			
			//draw zoomed dataset
			zoomedPlane = drawZoomedRow(indData, currCol, selectedBinNumber, thisZoomedRowData->last(), offset);
			thisZoomedRowData->removeLast();

			currCol += 1;
			addedRow = true;
			offset = offset + distanceToParent;
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
				
				for (int i = 0; i < cellIds->size(); i++)
				{//highlight each cell of the selected row
					
					highlightSelectedCell(thisAcc, cellIds->at(i));
				}

				//draw line border
				drawLineBetweenRowAndZoomedRow(zoomedPlane, originalPlane, cellIds);
			}

			offset = offset + distance;
			addedRow = false;
			offsetAlreadyAdded = true;
			originalPlaneZoomedPlanePair->insert({ m_originalPlaneActors->at(m_originalPlaneActors->size() - 1), m_zoomedPlaneActors->at(m_zoomedPlaneActors->size() - 1) });
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
	actor->GetProperty()->EdgeVisibilityOn();
	double col[3];
	col[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[0]; //iACompVisOptions::BACKGROUNDCOLOR_WHITE
	col[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[1]; //iACompVisOptions::BACKGROUNDCOLOR_WHITE
	col[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[2]; //iACompVisOptions::BACKGROUNDCOLOR_WHITE
	actor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
	m_renderer->AddActor(actor);

	m_originalPlaneActors->push_back(actor);

	//add name of dataset/row
	double pos[3] = { -m_rowSize*0.05, y + (m_colSize*0.5), 0.0 };
	addDatasetName(currDataInd, pos);

	return aPlane;
}

vtkSmartPointer<vtkPlaneSource> iACompHistogramTable::drawZoomedRow(
	int currDataInd, int currentColumn, int amountOfBins, bin::BinType* currentData, double offset)
{
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
	col[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[0]; //iACompVisOptions::BACKGROUNDCOLOR_WHITE
	col[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[1]; //iACompVisOptions::BACKGROUNDCOLOR_WHITE
	col[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[2]; //iACompVisOptions::BACKGROUNDCOLOR_WHITE
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
	int curSelectedActorId = 0;
	for (int currDataInd = 0; currDataInd < m_zoomedPlaneActors->size(); currDataInd++)
	{
		vtkSmartPointer<vtkActor> currAct = m_zoomedPlaneActors->at(currDataInd);

		vtkSmartPointer<vtkAlgorithm> algorithm = currAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
		vtkSmartPointer<vtkPlaneSource> plane = vtkPlaneSource::SafeDownCast(algorithm);

		int bins = m_zoomedRowData->at((m_zoomedRowData->size() - 1) - curSelectedActorId)->size();
		int resultingAmountBins = bins * selectedBinNumber;

		plane->SetXResolution(resultingAmountBins);
		plane->Update();

		vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
		colorData->SetName("colors");
		colorData->SetNumberOfComponents(3);

		colorRowForZoom(colorData, m_zoomedRowData->at((m_zoomedRowData->size() - 1) - curSelectedActorId), selectedBinNumber);
		plane->GetOutput()->GetCellData()->SetScalars(colorData);

		// Setup actor and mapper
		vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		mapper->SetInputConnection(plane->GetOutputPort());
		mapper->SetScalarModeToUseCellData();
		mapper->Update();
		currAct->Modified();

		curSelectedActorId++;
	}

	renderWidget();
}

void iACompHistogramTable::drawPointRepresentation()
{
	std::map<vtkSmartPointer<vtkActor>, vtkSmartPointer<vtkActor>>::iterator it;
	int diff = (m_amountDatasets - 1);
	int zoomedRowDataInd = m_zoomedRowData->size()-1;

	for (int indData = (m_amountDatasets - 1); indData >= 0; indData--)
	{
		int rowInd = diff - indData;
		
		it = originalPlaneZoomedPlanePair->find(m_originalPlaneActors->at(rowInd));
		if(it != originalPlaneZoomedPlanePair->end())
		{
			vtkSmartPointer<vtkActor> originalRowAct = it->first;
			vtkSmartPointer<vtkActor> zoomedRowAct = it->second;

			vtkSmartPointer<vtkAlgorithm> algorithm1 = originalRowAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
			vtkSmartPointer<vtkPlaneSource> originalPlane = vtkPlaneSource::SafeDownCast(algorithm1);

			vtkSmartPointer<vtkAlgorithm> algorithm = zoomedRowAct->GetMapper()->GetInputConnection(0, 0)->GetProducer();
			vtkSmartPointer<vtkPlaneSource> zoomedPlane = vtkPlaneSource::SafeDownCast(algorithm);

			double xmin = zoomedPlane->GetOrigin()[0];
			double xmax = zoomedPlane->GetPoint1()[0];
			double ymin = zoomedPlane->GetOrigin()[1];
			double ymax = zoomedPlane->GetPoint2()[1];
			double width = xmax - xmin;
			
			auto iter = m_pickedCellsforPickedRow->find(indData);
			if (iter == m_pickedCellsforPickedRow->end()) { continue; }
		
			std::vector<vtkIdType>* pickedCells = iter->second;
			double binLength = width / ((double)pickedCells->size());
			
			for (int cellId = 0; cellId < pickedCells->size(); cellId++)
			{
				//set plane
				vtkSmartPointer<vtkPlaneSource> pointPlane = vtkSmartPointer<vtkPlaneSource>::New();
				pointPlane->SetXResolution(1);
				pointPlane->SetYResolution(1);

				double newXMin = xmin + (binLength* cellId);
				double newXMax = newXMin + binLength;

				pointPlane->SetOrigin(newXMin, ymin, 0.0);
				pointPlane->SetPoint1(newXMax, ymin, 0.0);
				pointPlane->SetPoint2(newXMin, ymax, 0.0);
				pointPlane->Update();

				vtkSmartPointer<vtkPolyDataMapper> planeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
				planeMapper->SetInputConnection(pointPlane->GetOutputPort());
				planeMapper->SetScalarModeToUseCellData();
				planeMapper->Update();

				vtkSmartPointer<vtkActor> planeActor = vtkSmartPointer<vtkActor>::New();
				planeActor->SetMapper(planeMapper);
				planeActor->GetProperty()->SetEdgeVisibility(true);
				double col[3];
				col[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[0];
				col[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[1];
				col[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[2];
				planeActor->GetProperty()->SetEdgeColor(col[0], col[1], col[2]);
				m_pointRepresentationActors->push_back(planeActor);
				m_renderer->AddActor(planeActor);

				//set middle line
				double startP[3] = { newXMin, ymin + ((ymax - ymin) / 2.0), 0.0 };
				double endP[3] = { newXMax, ymin + ((ymax - ymin) / 2.0), 0.0 };
				double col1[3];
				col1[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[0];
				col1[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[1];
				col1[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[2];
				vtkSmartPointer<vtkActor> lineActor = drawLine(startP, endP , col1, 2);
				lineActor->GetProperty()->SetOpacity(0.1);
				lineActor->Modified();
				m_pointRepresentationActors->push_back(lineActor);
				
				//draw indiviudal data points
				double radius = (m_colSize*0.5)*0.25;
				double lineWidth = 1;
				double circleColor[3];
				circleColor[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[0];
				circleColor[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[1];
				circleColor[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[2];
				double lineColor[3];
				lineColor[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[0];
				lineColor[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[1];
				lineColor[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[2];

				std::vector<double> data = m_zoomedRowData->at(zoomedRowDataInd)->at(cellId);
				double newY = ymin + ((ymax - ymin) / 2.0);
				//(newXMin + radius) --> so that min & max points do not lie on border
				vtkSmartPointer<vtkPoints> points = calculatePointPosition(data, (newXMin + radius), (newXMax - radius), newY);
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
	for (unsigned int i = 0; i < m_pointRepresentationActors->size(); i++)
	{
		m_renderer->RemoveActor(m_pointRepresentationActors->at(i));
	}

	m_pointRepresentationActors->clear();
}

vtkSmartPointer<vtkPoints> iACompHistogramTable::calculatePointPosition(std::vector<double> dataPoints, double newMinX, double newMaxX, double y)
{
	if(dataPoints.size() == 0)
	{ //when the list is empty
		return nullptr;
	}

	vtkSmartPointer<vtkPoints> result = vtkSmartPointer<vtkPoints>::New();
	auto minMax = std::minmax_element(dataPoints.begin(), dataPoints.end());
	double min = *minMax.first;
	double max = *minMax.second;

	for (int i = 0; i < dataPoints.size(); i++)
	{
		double x = iACompVisOptions::histogramNormalization(dataPoints.at(i), newMinX, newMaxX, min, max);
		result->InsertNextPoint(x,y,0.0);
	}

	return result;
}

void iACompHistogramTable::drawLineBetweenRowAndZoomedRow(vtkSmartPointer<vtkPlaneSource> zoomedRowPlane, vtkSmartPointer<vtkPlaneSource> originalRowPlane, std::vector<vtkIdType>* cellIdsOriginalPlane)
{
	int amountOfCells = originalRowPlane->GetXResolution();
	double xMinO = originalRowPlane->GetOrigin()[0];
	double yMinO = originalRowPlane->GetOrigin()[1];
	double xMaxO = originalRowPlane->GetPoint1()[0];
	double widthO = xMaxO - xMinO;
	double binLengthO = widthO / ((double)amountOfCells);

	double xMinZ = zoomedRowPlane->GetOrigin()[0];
	double xMaxZ = zoomedRowPlane->GetPoint1()[0];
	double yMinZ = zoomedRowPlane->GetOrigin()[1];
	double yMaxZ = zoomedRowPlane->GetPoint2()[1];
	double widthZ = xMaxZ - xMinZ;
	double binLengthZ = widthZ/ ((double)(cellIdsOriginalPlane->size()));

	for (int i = 0; i < cellIdsOriginalPlane->size(); i++)
	{
		double currCellId = cellIdsOriginalPlane->at(i);

		double col[3];
		col[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[0];
		col[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[1];
		col[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[2];
		
		//left line
		double p0[3];
		p0[0] = xMinO + (binLengthO * currCellId);
		p0[1] = yMinO;
		p0[2] = 0.0;

		double p1[3];
		p1[0] = xMinZ + (binLengthZ * i);
		p1[1] = yMaxZ;
		p1[2] = 0.0;

		drawLine(p0, p1, col, iACompVisOptions::LINE_WIDTH);
		
		//right line
		double p2[3];
		p2[0] = p0[0] + binLengthO;
		p2[1] = p0[1];
		p2[2] = p0[2];

		double p3[3];
		p3[0] = p1[0] + binLengthZ;
		p3[1] = p1[1];
		p3[2] = p1[2];

		drawLine(p2, p3, col, iACompVisOptions::LINE_WIDTH);

		//border zoomed row
		double leftDown[3] = { p1[0], yMinZ, 0.0 };
		double rightDown[3] = { p3[0], yMinZ, 0.0 };
		double rightUp[3] = { p3[0], p3[1], p3[2] };
		double leftUp[3] = { p1[0], p1[1], p1[2] };

		vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
		points->InsertNextPoint(leftDown);
		points->InsertNextPoint(rightDown);
		points->InsertNextPoint(rightUp);
		points->InsertNextPoint(leftUp);
		points->InsertNextPoint(leftDown);
		drawPolyLine(points, col, iACompVisOptions::LINE_WIDTH);
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

	QColor c1 = QColor(77, 0, 23);
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
	double col[3];
	col[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[0];
	col[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[1];
	col[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY)[2];
	m_lut->SetBelowRangeColor(col[0], col[1], col[2], 1);
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
			continue;
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

	//init camera
	vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
	camera->SetPosition(m_rowSize*0.5, (m_colSize*m_amountDatasets)*0.5, 1);
	camera->SetFocalPoint( m_rowSize*0.5, (m_colSize*m_amountDatasets)*0.5, 0);
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

/******************************************  Data Caculation  **********************************************/
void iACompHistogramTable::calculateHistogramTable()
{
	m_histData = new iACompHistogramTableData(m_mds);
}

/******************************************  Interaction  **********************************************/
QList<bin::BinType*>* iACompHistogramTable::getSelectedData(Pick::PickedMap* map)
{
	QList<bin::BinType*>* binData = m_histData->getBinData();

	QList<bin::BinType*>* thisZoomedRowData = new QList<bin::BinType*>();
	m_indexOfPickedRow = new std::vector<int>();
	m_pickedCellsforPickedRow = new std::map<int, std::vector<vtkIdType>*>();

	int dataIndex = m_amountDatasets - 1;
	for (int currDataInd = m_originalPlaneActors->size() - 1; currDataInd >= 0; currDataInd--)
	{
		vtkSmartPointer<vtkActor> currAct = m_originalPlaneActors->at(currDataInd);

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

			m_indexOfPickedRow->push_back(abs((m_amountDatasets - 1) - dataIndex));
			m_pickedCellsforPickedRow->insert({ abs((m_amountDatasets - 1) - dataIndex), pickedCells});
		}

		dataIndex--;
	}

	//DEBUG
	DEBUG_LOG("DEBUGGING");
	for (int i = 0; i < thisZoomedRowData->size(); i++)
	{
		bin::debugBinType(thisZoomedRowData->at(i));
	}

	//store zoomed data as bin structure
	m_zoomedRowData = bin::DeepCopy(thisZoomedRowData);

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
	double col[3];
	col[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[0];
	col[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[1];
	col[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_BLACK)[2];
	selectedActor->GetProperty()->SetEdgeColor(col[0],col[1],col[2]);
	selectedActor->GetProperty()->SetLineWidth(iACompVisOptions::LINE_WIDTH);

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