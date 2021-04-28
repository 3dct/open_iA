#include "iACompTable.h"

//CompVis
#include "iACompHistogramVis.h"
#include "iACsvDataStorage.h"
#include "iACompUniformTableInteractorStyle.h"

//vtk
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkLookupTable.h"
#include "vtkActor.h"

iACompTable::iACompTable(iACompHistogramVis* vis) :
	m_vis(vis),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_lutDarker(vtkSmartPointer<vtkLookupTable>::New()),
	m_useDarkerLut(false),
	m_tableSize(10),
	m_mainRenderer(vtkSmartPointer<vtkRenderer>::New()),
	m_interactionStyle(vtkSmartPointer<iACompUniformTableInteractorStyle>::New()),
	m_rendererColorLegend(vtkSmartPointer<vtkRenderer>::New())
{
	initializeRenderer();
}

void iACompTable::initializeRenderer()
{
	m_mainRenderer->SetBackground(iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY));
	m_mainRenderer->SetViewport(0, 0, 0.8, 1);
	m_mainRenderer->SetUseFXAA(true);
}

void iACompTable::addDatasetName(int currDataset, double* position)
{
	QStringList* filenames = m_vis->getDataStorage()->getDatasetNames();
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
	legendProperty->SetColor(iACompVisOptions::getDoubleArray(iACompVisOptions::FONTCOLOR_TITLE));
	legendProperty->SetFontSize(iACompVisOptions::FONTSIZE_TITLE);
	legendProperty->SetVerticalJustificationToCentered();
	legendProperty->SetJustification(VTK_TEXT_RIGHT);
	legendProperty->Modified();

	m_mainRenderer->AddActor(legend);
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

void iACompTable::setInteractorStyleToWidget()
{
	m_vis->setInteractorStyleToWidget(m_interactionStyle);
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