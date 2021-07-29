#include "iACompVariableTableInteractorStyle.h"
#include <vtkObjectFactory.h> //for macro!

//CompVis
#include "iACompVariableTable.h"

//vtk
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPropPicker.h>
#include <vtkCellPicker.h>

#include <vtkActor.h>
#include <vtkProperty.h>

vtkStandardNewMacro(iACompVariableTableInteractorStyle);

iACompVariableTableInteractorStyle::iACompVariableTableInteractorStyle() : 
	iACompTableInteractorStyle(), 
	m_visualization(nullptr), 
	m_picker(vtkSmartPointer<vtkCellPicker>::New())
{
}

void iACompVariableTableInteractorStyle::setVariableTableVisualization(iACompVariableTable* visualization)
{
	m_visualization = visualization;
}

void iACompVariableTableInteractorStyle::OnLeftButtonDown()
{
	//set pick list
	setPickList(m_visualization->getOriginalRowActors());

	// Get the location of the click (in window coordinates)
	int* pos = this->GetInteractor()->GetEventPosition();
	this->FindPokedRenderer(pos[0], pos[1]);
	auto currentRenderer = this->GetDefaultRenderer();
	if (currentRenderer == nullptr)
	{
		LOG(lvlDebug, "VariableTableInteractorStyle: currentRenderer is null!!");
		return;
	}

	int cellPicked = m_picker->Pick(pos[0], pos[1], 0, this->CurrentRenderer);

	if (cellPicked == 0)
	{
		removeBarChart();
			//m_picked->clear();
			return;
	}
	
	if (cellPicked != NULL && this->GetInteractor()->GetShiftKey() && m_picker->GetActor()!= NULL)
	{
		vtkIdType id = m_picker->GetCellId();
		storePickedActorAndCell(m_picker->GetActor(), id);
		m_visualization->highlightSelectedCell(m_picker->GetActor(), id);
	}

	m_visualization->renderWidget();
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
	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	std::string key = interactor->GetKeySym();

	//when shift is released, the computation of the zoom is performed
	if (key == "Shift_L")
	{
		if (m_picked->size() >= 1)
		{
			m_visualization->removeHighlightedCells();

			//forward update to all other charts & histogram table
			//updateCharts();

			Pick::copyPickedMap(m_picked, m_pickedOld);

			//reset selection variables
			m_picked->clear();

			m_visualization->renderWidget();
			
		}
	}
}

void iACompVariableTableInteractorStyle::Pan()
{
}

bool iACompVariableTableInteractorStyle::removeBarChart()
{
	if (m_visualization->getBarChartAmountObjectsActive())
	{
		m_visualization->removeBarCharShowingAmountOfObjects();
		m_visualization->drawHistogramTable();
		m_visualization->renderWidget();
		return true;
	}

	return false;
}

void iACompVariableTableInteractorStyle::setPickList(std::vector<vtkSmartPointer<vtkActor>>* originalRowActors)
{
	m_picker->InitializePickList();

	for (int i = 0; i < originalRowActors->size(); i++)
	{
		m_picker->AddPickList(originalRowActors->at(i));
	}
}