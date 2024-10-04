// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARendererInteractorStyle.h"

#include <vtkObjectFactory.h>    // for vtkStandardNewMacro
#include <vtkRenderWindowInteractor.h>

void iAvtkInteractorStyleTrackballCamera::OnMouseWheelForward()
{
	if (GetInteractor()->GetControlKey() && GetInteractor()->GetShiftKey())
	{
		emit ctrlShiftMouseWheel(+1);
	}
	else if (!GetInteractor()->GetControlKey() && !GetInteractor()->GetAltKey()) // see iAFast3DMagicLensWidget
	{
		vtkInteractorStyleTrackballCamera::OnMouseWheelForward();
	}
}

void iAvtkInteractorStyleTrackballCamera::OnMouseWheelBackward()
{
	if (GetInteractor()->GetControlKey() && GetInteractor()->GetShiftKey())
	{
		emit ctrlShiftMouseWheel(-1);
	}
	else if (!GetInteractor()->GetControlKey() && !GetInteractor()->GetAltKey()) // see iAFast3DMagicLensWidget
	{
		vtkInteractorStyleTrackballCamera::OnMouseWheelBackward();
	}
}

void iAvtkInteractorStyleTrackballCamera::OnChar()
{
}

vtkStandardNewMacro(iAvtkInteractorStyleTrackballCamera);

iAvtkInteractorStyleTrackballCamera::iAvtkInteractorStyleTrackballCamera() = default;






void iAvtkInteractorStyleJoystickCamera::OnMouseWheelForward()
{
	if (GetInteractor()->GetControlKey() && GetInteractor()->GetShiftKey())
	{
		emit ctrlShiftMouseWheel(+1);
	}
	else if (!GetInteractor()->GetControlKey() && !GetInteractor()->GetAltKey()) // see iAFast3DMagicLensWidget
	{
		vtkInteractorStyleJoystickCamera::OnMouseWheelForward();
	}
}

void iAvtkInteractorStyleJoystickCamera::OnMouseWheelBackward()
{
	if (GetInteractor()->GetControlKey() && GetInteractor()->GetShiftKey())
	{
		emit ctrlShiftMouseWheel(-1);
	}
	else if (!GetInteractor()->GetControlKey() && !GetInteractor()->GetAltKey()) // see iAFast3DMagicLensWidget
	{
		vtkInteractorStyleJoystickCamera::OnMouseWheelBackward();
	}
}

void iAvtkInteractorStyleJoystickCamera::OnChar()
{
}

vtkStandardNewMacro(iAvtkInteractorStyleJoystickCamera);

iAvtkInteractorStyleJoystickCamera::iAvtkInteractorStyleJoystickCamera() = default;





void iAvtkInteractorStyleFlight::OnMouseWheelForward()
{
	if (GetInteractor()->GetControlKey() && GetInteractor()->GetShiftKey())
	{
		emit ctrlShiftMouseWheel(+1);
	}
	else if (!GetInteractor()->GetControlKey() && !GetInteractor()->GetAltKey()) // see iAFast3DMagicLensWidget
	{
		vtkInteractorStyleFlight::OnMouseWheelForward();
	}
}

void iAvtkInteractorStyleFlight::OnMouseWheelBackward()
{
	if (GetInteractor()->GetControlKey() && GetInteractor()->GetShiftKey())
	{
		emit ctrlShiftMouseWheel(-1);
	}
	else if (!GetInteractor()->GetControlKey() && !GetInteractor()->GetAltKey()) // see iAFast3DMagicLensWidget
	{
		vtkInteractorStyleFlight::OnMouseWheelBackward();
	}
}

void iAvtkInteractorStyleFlight::OnChar()
{
}

vtkStandardNewMacro(iAvtkInteractorStyleFlight);

iAvtkInteractorStyleFlight::iAvtkInteractorStyleFlight() = default;
