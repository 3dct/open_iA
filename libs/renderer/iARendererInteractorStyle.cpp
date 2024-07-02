// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARendererInteractorStyle.h"

#include <vtkObjectFactory.h>    // for vtkStandardNewMacro
#include <vtkRenderWindowInteractor.h>

void iARendererInteractorStyle::OnMouseWheelForward()
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

void iARendererInteractorStyle::OnMouseWheelBackward()
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

vtkStandardNewMacro(iARendererInteractorStyle);

iARendererInteractorStyle::iARendererInteractorStyle() = default;