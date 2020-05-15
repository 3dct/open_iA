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
	// Initialize with 0 = None
	std::vector<int> a(NUMBER_OF_OPTIONS, 0);
	std::vector<std::vector<int>> b(NUMBER_OF_ACTIONS, a);
	std::vector < std::vector<std::vector<int>>> c(NUMBER_OF_INPUTS, b);
	m_inputScheme = new std::vector < std::vector < std::vector<std::vector<int>>>>(NUMBER_OF_DEVICES, c);

	m_activeInput = new std::vector<int>(NUMBER_OF_DEVICES, -1);

}

void iAVRInteractorStyle::setVRMain(iAVRMain* vrMain)
{
	m_vrMain = vrMain;
}

// Calls, depending on Device - its input and action, the corresponding method
// Events can occure through left/right Controller and its input (trigger, grip, Trackpad,...) and an Action (Press, Release, Touch,...)
void iAVRInteractorStyle::OnButton3D(vtkEventData* edata)
{    
	// Used Device
	vtkEventDataDevice3D* device = edata->GetAsEventDataDevice3D();
	
	if (!device)
	{
		return;
	}
	vtkEventDataDevice deviceData = device->GetDevice();			// Controller
	vtkEventDataDeviceInput input = device->GetInput();              // Input Method
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
		m_vrMain->endInteraction(device, m_eventPosition, InteractionProp);
	}
	
}

inputScheme * iAVRInteractorStyle::getInputScheme()
{
	return m_inputScheme;
}

std::vector<int>* iAVRInteractorStyle::getActiveInput()
{
	return m_activeInput;
}
