// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAComp3DWidgetInteractionStyle.h"

#include "iAComp3DWidget.h"

//vtk
#include <vtkObjectFactory.h> //for macro!
#include <vtkRenderWindowInteractor.h>

vtkStandardNewMacro(iAComp3DWidgetInteractionStyle);

iAComp3DWidgetInteractionStyle::iAComp3DWidgetInteractionStyle():
	m_visualization(nullptr)
{

}

void iAComp3DWidgetInteractionStyle::setVisualization(iAComp3DWidget* visualization)
{
	m_visualization = visualization;
}

void iAComp3DWidgetInteractionStyle::OnLeftButtonDown()
{
	vtkInteractorStyleTrackballCamera::OnLeftButtonDown();

	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	if (interactor == nullptr) return;


	if (interactor->GetShiftKey())
	{
		m_visualization->resetWidget();
		m_visualization->renderWidget();
	}
}
