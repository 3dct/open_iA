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
	m_barTextActors(new std::vector<vtkSmartPointer<vtkTextActor>>())
{
	initializeRenderer();
}

void iACompTable::initializeRenderer()
{
	m_mainRenderer->SetBackground(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	m_mainRenderer->SetViewport(0, 0, 0.85, 1);
	m_mainRenderer->SetUseFXAA(true);
}

void iACompTable::addDatasetName(int currDataset, double* position)
{
	QStringList* filenames = m_vis->getDataStorage()->getDatasetNames();
	std::string name = filenames->at(currDataset).toLocal8Bit().constData();
	//erase path
	name.erase(0, name.find_last_of("/\\") + 1);
	//erase .csv
	size_t pos = name.find(".csv");
	name.erase(pos, name.length()-1);

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
	legendProperty->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TITLE));
	legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	legendProperty->SetVerticalJustificationToCentered();
	legendProperty->SetJustification(VTK_TEXT_RIGHT);
	legendProperty->Modified();

	m_mainRenderer->AddActor(legend);
}

///////////////////// NEW
void iACompTable::createBar(
	vtkSmartPointer<vtkPlaneSource> currPlane, int currAmountObjects, int maxAmountObjects)
{
	//calculate height of bar
	double maxHeight = currPlane->GetPoint2()[1] - currPlane->GetOrigin()[1];
	double height25 = currPlane->GetOrigin()[1] + (maxHeight * 0.25);
	double height75 = currPlane->GetOrigin()[1] + (maxHeight * 0.75);

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
	barPlane->SetPoint2(currPlane->GetPoint2()[0], height75, currPlane->GetPoint2()[2]);
	barPlane->Update();

	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(barPlane->GetOutputPort());
	mapper->Update();

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);
	double color[3];
	color[0] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN)[0];
	color[1] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN)[1];
	color[2] = iACompVisOptions::getDoubleArray(iACompVisOptions::HIGHLIGHTCOLOR_GREEN)[2];
	actor->GetProperty()->SetColor(color[0], color[1], color[2]);

	m_mainRenderer->AddActor(actor);
	m_barActors->push_back(actor);
}

void iACompTable::createAmountOfObjectsText(vtkSmartPointer<vtkPlaneSource> currPlane, int currAmountObjects)
{
	vtkSmartPointer<vtkTextActor> legend = vtkSmartPointer<vtkTextActor>::New();
	legend->SetTextScaleModeToNone();
	legend->SetInput(std::to_string(currAmountObjects).c_str());

	vtkSmartPointer<vtkTextProperty> legendProperty = legend->GetTextProperty();
	legendProperty->BoldOn();
	legendProperty->ItalicOff();
	legendProperty->ShadowOff();
	legendProperty->SetFontFamilyToArial();
	legendProperty->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_WHITE));
	legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TEXT);
	legendProperty->SetJustification(VTK_TEXT_LEFT);
	legendProperty->SetVerticalJustificationToCentered();
	legendProperty->Modified();

	double x = currPlane->GetPoint1()[0] + (m_vis->getRowSize() * 0.05);
	double height = currPlane->GetPoint2()[1] - currPlane->GetOrigin()[1];
	double y = currPlane->GetOrigin()[1] + (height * 0.5);
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
	scalarBar->GetFrameProperty()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_BLACK));
	scalarBar->SetBarRatio(1);

	//title properties
	scalarBar->GetTitleTextProperty()->SetLineOffset(50);
	scalarBar->GetTitleTextProperty()->BoldOn();
	scalarBar->GetTitleTextProperty()->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	scalarBar->GetTitleTextProperty()->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TITLE));
	scalarBar->GetTitleTextProperty()->SetVerticalJustificationToTop();
	scalarBar->GetTitleTextProperty()->SetBackgroundColor(
		iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	scalarBar->GetTitleTextProperty()->SetBackgroundOpacity(1);
	scalarBar->SetVerticalTitleSeparation(10);
	scalarBar->GetTitleTextProperty()->Modified();

	//text properties
	vtkSmartPointer<vtkTextProperty> propL = vtkSmartPointer<vtkTextProperty>::New();
	propL->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	propL->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TEXT));
	propL->Modified();
	scalarBar->SetAnnotationTextProperty(propL);

	//draw range below the colors (for 0)
	scalarBar->SetBelowRangeAnnotation("0");
	scalarBar->SetDrawBelowRangeSwatch(true);

	//draw background
	scalarBar->DrawBackgroundOn();
	scalarBar->GetBackgroundProperty()->SetColor(
		iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_BLACK));
	scalarBar->GetBackgroundProperty()->SetLineWidth(scalarBar->GetBackgroundProperty()->GetLineWidth() * 7);

	// Setup render window, renderer, and interactor
	m_rendererColorLegend->SetViewport(0.85, 0, 1, 1);
	m_rendererColorLegend->SetBackground(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
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



double iACompTable::round_up(double value, int decimal_places)
{
	const double multiplier = std::pow(10.0, decimal_places);
	return std::ceil(value * multiplier) / multiplier;
}