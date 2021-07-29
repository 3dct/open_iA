#include "iACompTableInteractorStyle.h"
#include <vtkObjectFactory.h>  //for macro!

//Debug
#include "iALog.h"
#include "iACompHistogramVis.h"

//VTK
#include <vtkSmartPointer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <vtkCamera.h>


namespace Pick
{
	void copyPickedMap(PickedMap* input, PickedMap* result)
	{
		std::map<vtkSmartPointer<vtkActor>, std::vector<vtkIdType>*>::iterator it;

		for (it = input->begin(); it != input->end(); it++)
		{
			vtkSmartPointer<vtkActor> currAc = it->first;
			std::vector<vtkIdType>* currVec = it->second;

			result->insert({currAc, currVec});
		}
	};

	void debugPickedMap(PickedMap* input)
	{
		LOG(lvlDebug, "#######################################################");
		LOG(lvlDebug, "");
		LOG(lvlDebug, "size = " + QString::number(input->size()));

		std::map<vtkSmartPointer<vtkActor>, std::vector<vtkIdType>*>::iterator it;
		int count = 0;
		for (it = input->begin(); it != input->end(); it++)
		{
			vtkSmartPointer<vtkActor> currAc = it->first;
			std::vector<vtkIdType>* currVec = it->second;

			LOG(lvlDebug,
				"Actor " + QString::number(count) + " has " + QString::number(currVec->size()) + " picked cells");
		}
		LOG(lvlDebug, "");
		LOG(lvlDebug, "#######################################################");
	};
}

vtkStandardNewMacro(iACompTableInteractorStyle);

iACompTableInteractorStyle::iACompTableInteractorStyle() : 
	m_main(nullptr), 
	m_zoomLevel(1), 
	m_zoomOn(true), 
	m_picked(new Pick::PickedMap()),
	m_pickedOld(new Pick::PickedMap())
{
}

void iACompTableInteractorStyle::setIACompHistogramVis(iACompHistogramVis* main)
{
	m_main = main;
}

void iACompTableInteractorStyle::OnLeftButtonDown()
{
}
void iACompTableInteractorStyle::OnLeftButtonUp()
{
}

void iACompTableInteractorStyle::OnMouseMove()
{
}

void iACompTableInteractorStyle::OnMiddleButtonDown()
{
}
void iACompTableInteractorStyle::OnRightButtonDown()
{
}

void iACompTableInteractorStyle::OnMouseWheelForward()
{
	//camera zoom in
	if (this->GetInteractor()->GetControlKey())
	{
		generalZoomIn();
		return;
	}
}

void iACompTableInteractorStyle::OnMouseWheelBackward()
{
	//camera zoom in
	if (this->GetInteractor()->GetControlKey())
	{
		generalZoomOut();
		return;
	}
}

void iACompTableInteractorStyle::OnKeyPress()
{
}
void iACompTableInteractorStyle::OnKeyRelease()
{
}

void iACompTableInteractorStyle::Pan()
{
}

void iACompTableInteractorStyle::generalZoomIn()
{
	m_main->getCamera()->Zoom(m_zoomLevel + 0.05);
	m_main->renderWidget();
}

void iACompTableInteractorStyle::generalZoomOut()
{
	m_main->getCamera()->Zoom(m_zoomLevel - 0.05);
	m_main->renderWidget();
}

void iACompTableInteractorStyle::storePickedActorAndCell(vtkSmartPointer<vtkActor> pickedA, vtkIdType id)
{
	if (m_picked->find(pickedA) != m_picked->end())
	{
		//when this actor has been picked already
		std::vector<vtkIdType>* v = m_picked->find(pickedA)->second;
		if (std::find(v->begin(), v->end(), id) == v->end())
		{  //when cellId is not already in the vector, add it
			v->push_back(id);
		}
	}
	else
	{
		//when this actor has NOT been picked until now
		std::vector<vtkIdType>* pickedCellsList = new std::vector<vtkIdType>();
		pickedCellsList->push_back(id);

		m_picked->insert({pickedA, pickedCellsList});
	}
}