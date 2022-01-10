#include "iACompTable.h"

//CompVis
#include "iACompHistogramVis.h"
#include "iACsvDataStorage.h"
#include "iACompTableInteractorStyle.h"

//vtk
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkLookupTable.h"
#include "vtkActor.h"
#include "vtkScalarBarActor.h"
#include "vtkProperty2D.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"

#include "vtkProgrammableGlyphFilter.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkCellData.h"

#include "vtkDoubleArray.h"



iACompTable::iACompTable(iACompHistogramVis* vis) :
	m_vis(vis),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_lutDarker(vtkSmartPointer<vtkLookupTable>::New()),
	m_useDarkerLut(false),
	m_tableSize(11),
	m_mainRenderer(vtkSmartPointer<vtkRenderer>::New()),
	m_rendererColorLegend(vtkSmartPointer<vtkRenderer>::New()),
	m_lastState(iACompVisOptions::lastState::Undefined),
	m_barActors(new std::vector<vtkSmartPointer<vtkActor>>()),
	m_barTextActors(new std::vector<vtkSmartPointer<vtkTextActor>>()),
	m_highlighingActors(new std::vector<vtkSmartPointer<vtkActor>>),
	m_indexOfPickedRow(new std::vector<int>()),
	m_pickedCellsforPickedRow(new std::map<int, std::vector<vtkIdType>*>()),
	m_zoomedRowData(nullptr),
	m_rowDataIndexPair(new std::map<vtkSmartPointer<vtkActor>, int>())
{
	initializeRenderer();
}

void iACompTable::initializeRenderer()
{
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col);
	m_mainRenderer->SetBackground(col);
	m_mainRenderer->SetViewport(0, 0, 0.85, 1);
	m_mainRenderer->SetUseFXAA(true);
}

/********************************************  Rendering ********************************************/
void iACompTable::constructBins(iACompHistogramTableData* data, bin::BinType* currRowData,
	vtkSmartPointer<vtkDoubleArray> originArray, vtkSmartPointer<vtkDoubleArray> point1Array,
	vtkSmartPointer<vtkDoubleArray> point2Array, vtkSmartPointer<vtkUnsignedCharArray> colorArray, int currDataInd,
	int currentColumn, double offset)
{
	//drawing positions
	double min_x = 0.0;
	double max_x = m_vis->getRowSize();
	double min_y = (m_vis->getColSize() * currentColumn) + offset;
	double max_y = min_y + m_vis->getColSize();

	//xOffset is added to prevent bins from overlapping
	double xOffset = 0.0;  //m_vis->getRowSize() * 0.5; //0.05

	double intervalStart = 0.0;
	std::vector<double> binBoundaries = data->getBinBoundaries()->at(currDataInd);
	int numberOfBins = binBoundaries.size();

	for (int i = 0; i < numberOfBins; i++)
	{
		double minVal = data->getMinVal();
		double maxVal = data->getMaxVal();
		//double lowerBoundary = binBoundaries.at(i);
		double upperBoundary = maxVal;
		if (i != (numberOfBins-1))
		{
			// 0.9999 required for correct drawing of vtkPlaneSource, when the bin only contains 1 value
			upperBoundary = binBoundaries.at(i + 1) * 0.999999;
		}
	
		double percentUpperBoundary = iACompVisOptions::calculatePercentofRange(upperBoundary, minVal, maxVal);

		//calculate min & max position for each bin
		double posXMin = intervalStart;
		double posXMax = iACompVisOptions::calculateValueAccordingToPercent(min_x, max_x, percentUpperBoundary);

		originArray->InsertTuple3(i, posXMin, min_y, 0.0);
		point1Array->InsertTuple3(i, (posXMax + xOffset) , min_y, 0.0);  //width
		point2Array->InsertTuple3(i, posXMin, max_y, 0.0);            // height

		/////////////////
		//// DEBUG INFO
		//// Check input
		//double v1[3], v2[3], normal[3];
		//double origin[3] = {posXMin, min_y, 0.0};
		//double point1[3] = {posXMax , min_y, 0.0};
		//double point2[3] = {posXMin, max_y, 0.0};

		//for (int k = 0; k < 3; k++)
		//{
		//	v1[k] = point1[k] - origin[k];
		//	v2[k] = point2[k] - origin[k];
		//}
		//
		//vtkMath::Cross(v1, v2, normal);
		//if (vtkMath::Normalize(normal) == 0.0)
		//{
		//	LOG(lvlDebug,
		//		"NOT Working : " + QString::number(normal[0]) + ", " + QString::number(normal[1]) + ", " +
		//			QString::number(normal[2]));
		//}
		/////////////////

		intervalStart = posXMax + xOffset;

		//calculate color for each bin
		double numberOfObjects = currRowData->at(i).size();
		double* rgbBorder;
		double rgb[3] = {0.0, 0.0, 0.0};
		double maxNumber = m_lut->GetRange()[1];
		double minNumber = m_lut->GetRange()[0];

		if (numberOfObjects > maxNumber)
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
			colorArray->InsertTuple3(i, ucrgb[0], ucrgb[1], ucrgb[2]);
		}
		else if (numberOfObjects < minNumber)
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
			colorArray->InsertTuple3(i, ucrgb[0], ucrgb[1], ucrgb[2]);
		}
		else
		{
			if (m_useDarkerLut)
			{
				m_lutDarker->GetColor(numberOfObjects, rgb);
			}
			else
			{
				m_lut->GetColor(numberOfObjects, rgb);
			}

			unsigned char ucrgb[3];
			iACompVisOptions::getColorArray3(rgb, ucrgb);
			colorArray->InsertTuple3(i, ucrgb[0], ucrgb[1], ucrgb[2]);
		}
	}

	//store drawing coordinates of bins
	vtkSmartPointer<vtkPoints> binPoints = vtkSmartPointer<vtkPoints>::New();
	binPoints->SetDataTypeToDouble();
	binPoints->SetNumberOfPoints(numberOfBins);

	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->SetPoints(binPoints);
	polydata->GetPointData()->AddArray(originArray);
	polydata->GetPointData()->AddArray(point1Array);
	polydata->GetPointData()->AddArray(point2Array);
	polydata->GetCellData()->AddArray(colorArray);
	polydata->GetCellData()->SetActiveScalars("colorArray");

	data->storeBinPolyData(polydata);
}

void buildGlyphRepresentation(void* arg)
{
	vtkProgrammableGlyphFilter* glyphFilter = (vtkProgrammableGlyphFilter*)arg;
	double origin[3];
	double point1[3];
	double point2[3];

	int pid = glyphFilter->GetPointId();
	glyphFilter->GetPointData()->GetArray("originArray")->GetTuple(pid, origin);
	glyphFilter->GetPointData()->GetArray("point1Array")->GetTuple(pid, point1);
	glyphFilter->GetPointData()->GetArray("point2Array")->GetTuple(pid, point2);

	vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
	plane->SetXResolution(1);
	plane->SetYResolution(1);

	plane->SetOrigin(origin);
	plane->SetPoint1(point1);
	plane->SetPoint2(point2);
	plane->Update();

	glyphFilter->SetSourceData(plane->GetOutput());
}

void iACompTable::addDatasetName(int currDataset, double* position)
{
	QStringList* filenames = m_vis->getDataStorage()->getDatasetNames();
	std::string name = filenames->at(currDataset).toLocal8Bit().constData();
	//erase path
	name.erase(0, name.find_last_of("/\\") + 1);
	//erase .csv
	size_t pos = name.find(".csv");
	name.erase(pos, name.length() - 1);

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

	m_mainRenderer->AddActor(legend);
}

/********************************************  Ordering/Ranking ********************************************/
void iACompTable::createBarChart(vtkSmartPointer<vtkPolyData> currPolyData, int currAmountObjects, int maxAmountObjects)
{
	vtkSmartPointer<vtkDoubleArray> originArray = vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("originArray"));
	vtkSmartPointer<vtkDoubleArray> point1Array = vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point1Array"));
	vtkSmartPointer<vtkDoubleArray> point2Array = vtkDoubleArray::SafeDownCast(currPolyData->GetPointData()->GetArray("point2Array"));

	double origin[3];
	originArray->GetTuple(0, origin);

	double point1[3];
	point1Array->GetTuple(point1Array->GetNumberOfTuples()-1, point1);

	double point2[3];
	point2Array->GetTuple(0, point2);

	//calculate height of bar
	double maxHeight = point2[1] - origin[1];
	double height25 = origin[1] + (maxHeight * 0.25);
	double height75 = origin[1] + (maxHeight * 0.75);
	
	//calculate width of bar
	double maxWidth = point1[0] - origin[0];
	double percent = ((double)currAmountObjects) / ((double)maxAmountObjects);
	double correctWidth = maxWidth * percent;
	double correctX = origin[0] + correctWidth;

	double positions[4] = 
	{
		origin[0], correctX,  //x_min, x_max
		height25, height75                    //y_min, y_max
	};

	createBars(positions);

	double textPosition[3] = {
								point1[0],  //x_max
								origin[1],  //y_min
								point2[1]   //y_max
							 };

	createAmountOfObjectsText(textPosition, currAmountObjects);
}

void iACompTable::createBarChart(double* positions, int currAmountObjects, int maxAmountObjects)
{
	double x_min = positions[0];
	double x_max = positions[1];
	double y_min = positions[2];
	double y_max = positions[3];
	
	//calculate height of bar
	double maxHeight = y_max - y_min;
	double height25 = y_min + (maxHeight * 0.25);
	double height75 = y_min + (maxHeight * 0.75);

	//calculate width of bar//calculate width of bar
	double maxWidth = x_max - x_min;
	double percent = ((double)currAmountObjects) / ((double)maxAmountObjects);
	double correctWidth = maxWidth * percent;
	double correctX = x_min + correctWidth;

	double values[4] = {
		x_min, correctX,     //x_min, x_max
		height25, height75   //y_min, y_max
	};

	createBars(values);

	double textPosition[3] = {x_max, y_min, y_max};
	createAmountOfObjectsText(textPosition, currAmountObjects);
}

void iACompTable::createBars(double* positions)
{
	double x_min = positions[0];
	double x_max = positions[1];
	double y_min = positions[2];
	double y_max = positions[3];

	vtkSmartPointer<vtkPlaneSource> barPlane = vtkSmartPointer<vtkPlaneSource>::New();
	barPlane->SetXResolution(1);
	barPlane->SetYResolution(1);
	barPlane->SetOrigin(x_min, y_min, 0.0);
	barPlane->SetPoint1(x_max, y_min, 0.0);
	barPlane->SetPoint2(x_min, y_max, 0.0);
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

	m_mainRenderer->AddActor(actor);
	m_barActors->push_back(actor);
}

void iACompTable::createAmountOfObjectsText(double positions[3], int currAmountObjects)
{
	double x_max = positions[0];
	double y_min = positions[1];
	double y_max = positions[2];

	//calculate position
	double x = x_max + (m_vis->getRowSize() * 0.05);
	double height = y_max - y_min;
	double y = y_min + (height * 0.5);

	vtkSmartPointer<vtkTextActor> legend = vtkSmartPointer<vtkTextActor>::New();
	legend->SetTextScaleModeToNone();
	legend->SetInput(std::to_string(currAmountObjects).c_str());

	vtkSmartPointer<vtkTextProperty> legendProperty = legend->GetTextProperty();
	legendProperty->BoldOn();
	legendProperty->ItalicOff();
	legendProperty->ShadowOff();
	legendProperty->SetFontFamilyToArial();
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE, col);
	legendProperty->SetColor(col);
	legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	legendProperty->SetJustification(VTK_TEXT_LEFT);
	legendProperty->SetVerticalJustificationToCentered();
	legendProperty->Modified();

	legend->GetPositionCoordinate()->SetCoordinateSystemToWorld();
	legend->GetPositionCoordinate()->SetValue(x, y);
	legend->Modified();

	m_mainRenderer->AddActor(legend);
	m_barTextActors->push_back(legend);
}

void iACompTable::removeBarCharShowingAmountOfObjects()
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

bool iACompTable::getBarChartAmountObjectsActive()
{
	return m_useDarkerLut;
}

/********************************************  Legend Initialization ********************************************/
std::string iACompTable::initializeLegendLabels(std::string input)
{
	std::string result;
	std::string helper = input;
	std::string newLow = input.erase(input.find('.'), std::string::npos);

	if (newLow.size() > 1)
	{  //more than one charachater before the dot
		result = newLow;
	}
	else
	{  //only one character before the dot
		//result = helper.erase(helper.find('.') + 2, std::string::npos); // round to 2 decimal
		result = newLow;
	}

	return result;
}

void iACompTable::initializeLegend()
{
	vtkSmartPointer<vtkScalarBarActor> scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
	scalarBar->SetLookupTable(m_lut);
	scalarBar->SetHeight(0.9);  //scalarBar is set so high, that the blacckground above the title cannot be seen anymore
	scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	scalarBar->GetPositionCoordinate()->SetValue(0.40, 0.15, 0.0);
	scalarBar->SetWidth(0.15);
	scalarBar->SetUnconstrainedFontSize(1);

	scalarBar->SetTitle("Number of Objects");
	scalarBar->SetNumberOfLabels(0);
	scalarBar->SetTextPositionToPrecedeScalarBar();

	//draw frame around colored bars
	scalarBar->DrawFrameOn();
	scalarBar->GetFrameProperty()->SetLineWidth(scalarBar->GetFrameProperty()->GetLineWidth() * 6);
	scalarBar->GetFrameProperty()->SetDisplayLocationToForeground();
	double col[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_BLACK, col);
	scalarBar->GetFrameProperty()->SetColor(col);
	scalarBar->SetBarRatio(1);

	//title properties
	scalarBar->GetTitleTextProperty()->SetLineOffset(50);
	scalarBar->GetTitleTextProperty()->BoldOn();
	scalarBar->GetTitleTextProperty()->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	
	double col1[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TITLE, col1);
	scalarBar->GetTitleTextProperty()
		->SetColor(col1);
	scalarBar->GetTitleTextProperty()->SetVerticalJustificationToTop();
	
	double col2[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col2);
	scalarBar->GetTitleTextProperty()->SetBackgroundColor(col2);
	scalarBar->GetTitleTextProperty()->SetBackgroundOpacity(1);
	scalarBar->SetVerticalTitleSeparation(10);
	scalarBar->GetTitleTextProperty()->Modified();

	//text properties
	vtkSmartPointer<vtkTextProperty> propL = vtkSmartPointer<vtkTextProperty>::New();
	propL->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	double col3[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TEXT, col3);
	propL->SetColor(col3);
	propL->Modified();
	scalarBar->SetAnnotationTextProperty(propL);

	//draw range below the colors (for 0)
	scalarBar->SetBelowRangeAnnotation("0");
	scalarBar->SetDrawBelowRangeSwatch(true);

	//draw background
	scalarBar->DrawBackgroundOn();
	double col4[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_BLACK, col4);
	scalarBar->GetBackgroundProperty()->SetColor(col4);
	scalarBar->GetBackgroundProperty()->SetLineWidth(scalarBar->GetBackgroundProperty()->GetLineWidth() * 7);

	// Setup render window, renderer, and interactor
	m_rendererColorLegend->SetViewport(0.85, 0, 1, 1);
	double col5[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col5);
	m_rendererColorLegend->SetBackground(col5);
	m_rendererColorLegend->AddActor2D(scalarBar);
}

/****************************************** Getter & Setter **********************************************/

iACompHistogramVis* iACompTable::getHistogramVis()
{
	return m_vis;
}

void iACompTable::addRendererToWidget()
{
	m_vis->addRendererToWidget(m_mainRenderer);
}

void iACompTable::addLegendRendererToWidget()
{
	m_vis->addRendererToWidget(m_rendererColorLegend);
}

void iACompTable::setInteractorStyleToWidget(vtkSmartPointer<iACompTableInteractorStyle> interactorStyle)
{
	m_vis->setInteractorStyleToWidget(interactorStyle);
	
}

void iACompTable::renderWidget()
{
	m_vis->renderWidget();
}

vtkSmartPointer<vtkRenderer> iACompTable::getRenderer()
{
	return m_mainRenderer;
}

std::vector<int>* iACompTable::getIndexOfPickedRows()
{
	return m_indexOfPickedRow;
}

double iACompTable::round_up(double value, int decimal_places)
{
	const double multiplier = std::pow(10.0, decimal_places);
	return std::ceil(value * multiplier) / multiplier;
}

void iACompTable::removeHighlightedCells()
{
	for (int i = 0; i < m_highlighingActors->size(); i++)
	{
		m_mainRenderer->RemoveActor(m_highlighingActors->at(i));
	}

	m_highlighingActors->clear();
}