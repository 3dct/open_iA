#include "iACompVariableTableInteractorStyle.h"
#include <vtkObjectFactory.h> //for macro!

//CompVis
#include "iACompVariableTable.h"

//vtk
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPropPicker.h>

vtkStandardNewMacro(iACompVariableTableInteractorStyle);

iACompVariableTableInteractorStyle::iACompVariableTableInteractorStyle() : 
	iACompTableInteractorStyle(), 
	m_visualization(nullptr), 
	m_picker(vtkSmartPointer<vtkPropPicker>::New())
{
}

void iACompVariableTableInteractorStyle::setVariableTableVisualization(iACompVariableTable* visualization)
{
	m_visualization = visualization;
}

void iACompVariableTableInteractorStyle::OnLeftButtonDown()
{
	//set pick list
	//setPickList(m_visualization->getOriginalRowActors());

	// Get the location of the click (in window coordinates)
	int* pos = this->GetInteractor()->GetEventPosition();
	this->FindPokedRenderer(pos[0], pos[1]);
	auto currentRenderer = this->GetDefaultRenderer();
	if (currentRenderer == nullptr)
	{
		LOG(lvlDebug, "HistogramTableInteractorStyle: currentRenderer is null!!");
		return;
	}

	int is = m_picker->Pick(pos[0], pos[1], 0, this->CurrentRenderer);
	if (is == 0)
	{
		resetVariableTable();
		//m_picked->clear();
		return;
	}
}
void iACompVariableTableInteractorStyle::OnLeftButtonUp()
{
}

void iACompVariableTableInteractorStyle::OnMouseMove()
{
}

void iACompVariableTableInteractorStyle::OnMiddleButtonDown()
{
}
void iACompVariableTableInteractorStyle::OnRightButtonDown()
{
}

void iACompVariableTableInteractorStyle::OnMouseWheelForward()
{
	//camera zoom in
	if (this->GetInteractor()->GetControlKey())
	{
		generalZoomIn();
		return;
	}
}

void iACompVariableTableInteractorStyle::OnMouseWheelBackward()
{
	//camera zoom in
	if (this->GetInteractor()->GetControlKey())
	{
		generalZoomOut();
		return;
	}
}

void iACompVariableTableInteractorStyle::OnKeyPress()
{
}
void iACompVariableTableInteractorStyle::OnKeyRelease()
{
}

void iACompVariableTableInteractorStyle::Pan()
{
}

void iACompVariableTableInteractorStyle::resetVariableTable()
{
	if (m_visualization->getBarChartAmountObjectsActive())
	{
		m_visualization->removeBarCharShowingAmountOfObjects();
	}

	m_visualization->drawHistogramTable();

	m_visualization->renderWidget();
}