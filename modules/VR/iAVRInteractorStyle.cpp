/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAVRInteractorStyle.h"

#include <iAConsole.h>
#include <vtkObjectFactory.h>
#include "vtkOpenVRRenderWindowInteractor.h"
#include <vtkPropPicker.h>
#include <vtkPointPicker.h>
#include "iAVRMain.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"

vtkStandardNewMacro(iAVRInteractorStyle);

//Reimplements the Constructor
iAVRInteractorStyle::iAVRInteractorStyle()
{

/*	for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
	{
		this->InteractionState[d] = 0; // static_cast<int>(iAVROperations::None)
		this->InteractionProps[d] = nullptr;
		this->ClippingPlanes[d] = nullptr;

		for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
		{
			this->InputMap[d][i] = -1;
			//this->ControlsHelpers[d][i] = nullptr;
		}
	}

	vtkNew<vtkPolyDataMapper> pdm;
	this->PickActor->SetMapper(pdm);
	this->PickActor->GetProperty()->SetLineWidth(4);
	this->PickActor->GetProperty()->RenderLinesAsTubesOn();
	this->PickActor->GetProperty()->SetRepresentationToWireframe();
	this->PickActor->DragableOff();

	this->HoverPickOff();
	*/
}

void iAVRInteractorStyle::setVRMain(iAVRMain* vrMain)
{
	m_vrMain = vrMain;
	DEBUG_LOG(QString::number(vrMain->octreeLevel));
	DEBUG_LOG(QString::number(m_vrMain->octreeLevel));
}

// Calls, depending on Device - its input and action, the corresponding method
// Events can occure through left/right Controller and its input (trigger, grip, Trackpad,...) and an Action (Press, Release, Touch,...)
void iAVRInteractorStyle::OnButton3D(vtkEventData* edata)
{
	//vtkOpenVRInteractorStyle::OnButton3D(edata);
    
	// Used Device
	vtkEventDataDevice3D* device = edata->GetAsEventDataDevice3D();

	if (!device)
	{
		return;
	}

	//vtkEventDataDeviceInput input = device->GetInput();              // Input Method
	vtkEventDataAction action = device->GetAction();                 // Action of Input Method

	// TODO Performance?
	int x = this->Interactor->GetEventPosition()[0];
	int y = this->Interactor->GetEventPosition()[1];
	this->FindPokedRenderer(x, y);

	device->GetWorldPosition(m_eventPosition);

	this->FindPickedActor(m_eventPosition, nullptr);


	if(action == vtkEventDataAction::Press || action == vtkEventDataAction::Touch)
	{
		m_vrMain->startInteraction(device, m_eventPosition, InteractionProp);

	}
	else if(action == vtkEventDataAction::Release || action == vtkEventDataAction::Untouch)
	{
		m_vrMain->endInteraction();
	}
	
}
