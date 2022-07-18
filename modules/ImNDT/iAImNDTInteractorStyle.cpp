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
#include "iAImNDTInteractorStyle.h"

#include "iAImNDTMain.h"

#include <iALog.h>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkOpenVRModel.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkPointPicker.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPropPicker.h>
#include <vtkTransform.h>
#include <vtkVRMenuWidget.h>

vtkStandardNewMacro(iAImNDTInteractorStyle);

//! Reimplements the Constructor
iAImNDTInteractorStyle::iAImNDTInteractorStyle()
{
	// Initialize with 0 = None
	std::vector<int> a(NUMBER_OF_OPTIONS, 0);
	std::vector<std::vector<int>> b(NUMBER_OF_ACTIONS, a);
	std::vector < std::vector<std::vector<int>>> c(NUMBER_OF_INPUTS, b);
	m_inputScheme = new std::vector < std::vector < std::vector<std::vector<int>>>>(NUMBER_OF_DEVICES, c);
	
	m_activeInput = new std::vector<int>(NUMBER_OF_DEVICES, -1);
}

void iAImNDTInteractorStyle::setVRMain(iAImNDTMain* vrMain)
{
	m_vrMain = vrMain;
}

//! Calls, depending on Device - its input and action, the corresponding method
//! Events can occure through left/right Controller and its input (trigger, grip, Trackpad,...) and an Action (Press, Release, Touch,...)
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 1, 0)
void iAImNDTInteractorStyle::OnButton3D(vtkEventData* edata)
#else
void iAImNDTInteractorStyle::OnSelect3D(vtkEventData* edata)
{
#endif
	// Used Device
	vtkEventDataDevice3D* device = edata->GetAsEventDataDevice3D();
	
	if (!device)
	{
		return;
	}
	//vtkEventDataDevice deviceData = device->GetDevice();			// Controller
	//vtkEventDataDeviceInput input = device->GetInput();              // Input Method
	vtkEventDataAction action = device->GetAction();                 // Action of Input Method

	// TODO Performance?
	int x = this->Interactor->GetEventPosition()[0];
	int y = this->Interactor->GetEventPosition()[1];
	this->FindPokedRenderer(x, y);

	device->GetWorldPosition(m_eventPosition);
	device->GetWorldOrientation(m_eventOrientation);

	m_eventOrientation[1] = vtkMath::DegreesFromRadians(m_eventOrientation[1]);
	m_eventOrientation[2] = vtkMath::DegreesFromRadians(m_eventOrientation[2]);
	m_eventOrientation[3] = vtkMath::DegreesFromRadians(m_eventOrientation[3]);

	this->PickingManagedOff();
	this->FindPickedActor(m_eventPosition, nullptr);

	if(action == vtkEventDataAction::Press || action == vtkEventDataAction::Touch)
	{
		m_vrMain->startInteraction(device, InteractionProp, m_eventPosition, m_eventOrientation);
	}
	else if(action == vtkEventDataAction::Release || action == vtkEventDataAction::Untouch)
	{
		m_vrMain->endInteraction(device, InteractionProp, m_eventPosition, m_eventOrientation);
	}	
}

//! Is called when a Controller moves. Forwards the event to the main class
void iAImNDTInteractorStyle::OnMove3D(vtkEventData * edata)
{
	// Used Device
	vtkEventDataDevice3D* device = edata->GetAsEventDataDevice3D();

	int x = this->Interactor->GetEventPosition()[0];
	int y = this->Interactor->GetEventPosition()[1];
	this->FindPokedRenderer(x, y);

	device->GetWorldPosition(m_movePosition);
	device->GetWorldOrientation(m_eventOrientation);

	/* //Orientation?
	vtkRenderer* ren = this->CurrentRenderer;
	vtkOpenVRRenderWindow* renWin =
		vtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
	vtkOpenVRRenderWindowInteractor* iren =
		static_cast<vtkOpenVRRenderWindowInteractor*>(this->Interactor);

	vtkOpenVRModel* cmodel = renWin->GetTrackedDeviceModel(device->GetDevice());

	renWin->GetTrackedDevicePose(cmodel->TrackedDevice);
	*/

	m_eventOrientation[1] = vtkMath::DegreesFromRadians(m_eventOrientation[1]);
	m_eventOrientation[2] = vtkMath::DegreesFromRadians(m_eventOrientation[2]);
	m_eventOrientation[3] = vtkMath::DegreesFromRadians(m_eventOrientation[3]);

	//this->FindPickedActor(m_movePosition, nullptr);

	m_vrMain->onMove(device, m_movePosition, m_eventOrientation);

}

void iAImNDTInteractorStyle::OnPinch()
{
	double dyf = this->Interactor->GetScale() / this->Interactor->GetLastScale();
	vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
	vtkRenderWindowInteractor3D* rwi = static_cast<vtkRenderWindowInteractor3D*>(this->Interactor);
	double physicalScale = rwi->GetPhysicalScale();
	this->SetScale(camera, physicalScale / dyf);

	m_vrMain->onZoom();
}

//! Returns a vector for the input scheme
//! For every [device] an [inputID] and its [action] on an selection [option] a specific interaction is specified
inputScheme * iAImNDTInteractorStyle::getInputScheme()
{
	return m_inputScheme;
}

std::vector<int>* iAImNDTInteractorStyle::getActiveInput()
{
	return m_activeInput;
}

//! Calculates the vector of the diagonals in the touchpad "square" and returns if the position is Up, Right, Down or left on the pad.
//! The touchpad forms a square from (-1,-1) to (1,1)
iAVRTouchpadPosition iAImNDTInteractorStyle::getTouchedPadSide(float position[3])
{
	vtkVector2f p0 = vtkVector2f(-1,-1);
	vtkVector2f p1 = vtkVector2f(1, 1);

	vtkVector2f q0 = vtkVector2f(1, -1);
	vtkVector2f q1 = vtkVector2f(-1, 1);

	float sideOfDiag1 = ((p1.GetX() - p0.GetX()) * (position[1] - p0.GetY()) - (p1.GetY() - p0.GetY()) * (position[0] - p0.GetX()));
	float sideOfDiag2 = ((q1.GetX() - q0.GetX()) * (position[1] - q0.GetY()) - (q1.GetY() - q0.GetY()) * (position[0] - q0.GetX()));

	if (sideOfDiag1 > 0 && sideOfDiag2 > 0) return iAVRTouchpadPosition::Left;
	if (sideOfDiag1 > 0 && sideOfDiag2 < 0) return iAVRTouchpadPosition::Up;
	if (sideOfDiag1 < 0 && sideOfDiag2 > 0) return iAVRTouchpadPosition::Down;
	if (sideOfDiag1 < 0 && sideOfDiag2 < 0) return iAVRTouchpadPosition::Right;

	return iAVRTouchpadPosition::Unknown;
}

iAVRViewDirection iAImNDTInteractorStyle::getViewDirection(double viewDir[3])
{
	double maxVal = 0;
	int maxDir = -1;

	for (int i = 0; i < 3; i++)
	{
		if(maxVal < abs(viewDir[i]))
		{
			maxVal = abs(viewDir[i]);
			maxDir = i;
		}
	}

	if (maxDir == 0 && viewDir[maxDir] > 0) return iAVRViewDirection::Right;
	if (maxDir == 0 && viewDir[maxDir] < 0) return iAVRViewDirection::Left;
	if (maxDir == 1 && viewDir[maxDir] > 0) return iAVRViewDirection::Up;
	if (maxDir == 1 && viewDir[maxDir] < 0) return iAVRViewDirection::Down;
	if (maxDir == 2 && viewDir[maxDir] > 0) return iAVRViewDirection::Backwards;
	if (maxDir == 2 && viewDir[maxDir] < 0) return iAVRViewDirection::Forward;

	return iAVRViewDirection::Unknown;
}
