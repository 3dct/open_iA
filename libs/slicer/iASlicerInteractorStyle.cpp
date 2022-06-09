/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iASlicerInteractorStyle.h"

#include <vtkActor2D.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLine.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkUnsignedCharArray.h>

#include <iALog.h>
#include <iAMathUtility.h>

void iASlicerInteractionEvents::triggerSelection(int dragStart[2], int dragEnd[2])
{
	emit selection(dragStart, dragEnd);
}

vtkStandardNewMacro(iASlicerInteractorStyle);

iASlicerInteractorStyle::iASlicerInteractorStyle() :
	m_rightButtonDragZoomEnabled(true),
	m_leftButtonDown(false),
	m_interactionMode(imNormal),
	m_selRectPolyData(vtkSmartPointer<vtkPolyData>::New()),
	m_selRectMapper(vtkSmartPointer<vtkPolyDataMapper2D>::New()),
	m_selRectActor(vtkSmartPointer<vtkActor2D>::New())
{
	m_dragStart[0] = m_dragStart[1] = 0;

	// TODO: avoid duplication between here and selection interactor in Fiaker; maybe extract 2D rectangle "widget" class?
	vtkNew<vtkPoints> pts;
	pts->InsertNextPoint(0, 0, 0);
	pts->InsertNextPoint(0, 0, 0);
	pts->InsertNextPoint(0, 0, 0);
	pts->InsertNextPoint(0, 0, 0);
	m_selRectPolyData->SetPoints(pts.GetPointer());

	vtkNew<vtkCellArray> lines;
	for (int i = 0; i < 4; ++i)
	{
		vtkNew<vtkLine> line;
		line->GetPointIds()->SetId(0, i);
		line->GetPointIds()->SetId(1, (i + 1) % 4);
		lines->InsertNextCell(line.GetPointer());
	}
	m_selRectPolyData->SetLines(lines.GetPointer());

	vtkNew<vtkUnsignedCharArray> colors;
	colors->SetNumberOfComponents(3);
	double color[3] = {255, 0.0, 0.0};
	for (int i = 0; i < 4; ++i)
	{
		colors->InsertNextTuple(color);
	}
	m_selRectPolyData->GetCellData()->SetScalars(colors.GetPointer());
	m_selRectMapper->SetInputData(m_selRectPolyData);
	m_selRectActor->GetProperty()->SetColor(1, 0, 0);
	m_selRectActor->GetProperty()->SetOpacity(1);
	m_selRectActor->GetProperty()->SetLineWidth(2.0);
	m_selRectActor->SetMapper(m_selRectMapper);
	m_selRectActor->SetPickable(false);
	m_selRectActor->SetDragable(false);
}

void iASlicerInteractorStyle::OnLeftButtonDown()
{
	// TODO: find more reliable way to determine whether left button currently pressed!
	//! With m_leftButtonDown it does not work properly if release happens outside!
	m_leftButtonDown = true;

	// if no modifier key pressed:
	if (!Interactor->GetShiftKey() && !Interactor->GetControlKey() && !Interactor->GetAltKey())
	{
		switch (m_interactionMode)
		{
		default:    // fall through
		case imNormal:
			break;	// no handling

		// if enabled, start "window-level" (click+drag) interaction:
		case imWindowLevelAdjust:
		{	// mostly copied from base class; but we don't want the "GrabFocus" call there,
			// that prevents the listeners to be notified of mouse move calls
			int x = this->Interactor->GetEventPosition()[0];
			int y = this->Interactor->GetEventPosition()[1];

			this->FindPokedRenderer(x, y);
			if (this->CurrentRenderer == nullptr)
			{
				return;
			}
			if (!this->Interactor->GetShiftKey() && !this->Interactor->GetControlKey())
			{
				this->WindowLevelStartPosition[0] = x;
				this->WindowLevelStartPosition[1] = y;
				this->StartWindowLevel();
			}
			break;
		}
		case imRegionSelect:
		{
			auto renWin = this->Interactor->GetRenderWindow();
			renWin->GetRenderers()->GetFirstRenderer()->AddActor(m_selRectActor);
			m_dragStart[0] = this->Interactor->GetEventPosition()[0];
			m_dragStart[1] = this->Interactor->GetEventPosition()[1];
			m_dragEnd[0] = m_dragStart[0];
			m_dragEnd[1] = m_dragStart[1];
			updateSelectionRect();
			break;
		}
		}
	}

	// from the interaction possibilities in vtkInteractorStyleImage, only allow moving the slice (Shift+Drag):
	if (!this->Interactor->GetShiftKey())
	{
		return;
	}
	vtkInteractorStyleImage::OnLeftButtonDown();
}
void iASlicerInteractorStyle::OnMouseMove()
{
	if (m_leftButtonDown && m_interactionMode == imRegionSelect &&
		!Interactor->GetShiftKey() && !Interactor->GetControlKey() && !Interactor->GetAltKey())
	{
		int const* size = this->Interactor->GetRenderWindow()->GetSize();
		m_dragEnd[0] = clamp(0, size[0] - 1, this->Interactor->GetEventPosition()[0]);
		m_dragEnd[1] = clamp(0, size[1] - 1, this->Interactor->GetEventPosition()[1]);
		updateSelectionRect();
	}
	else
	{
		vtkInteractorStyleImage::OnMouseMove();
	}
}
void iASlicerInteractorStyle::OnLeftButtonUp()
{
	if (this->State == VTKIS_WINDOW_LEVEL)
	{
		this->EndWindowLevel();
	}
	else if (m_leftButtonDown && m_interactionMode == imRegionSelect &&
		!Interactor->GetShiftKey() && !Interactor->GetControlKey() && !Interactor->GetAltKey())
	{
		int const* size = this->Interactor->GetRenderWindow()->GetSize();
		m_dragEnd[0] = clamp(0, size[0] - 1, this->Interactor->GetEventPosition()[0]);
		m_dragEnd[1] = clamp(0, size[1] - 1, this->Interactor->GetEventPosition()[1]);
		auto renWin = this->Interactor->GetRenderWindow();
		renWin->GetRenderers()->GetFirstRenderer()->RemoveActor(m_selRectActor);
		// ... handle actual event with given rectangle...
		m_events.triggerSelection(m_dragStart, m_dragEnd);
	}
	m_leftButtonDown = false;
	vtkInteractorStyleImage::OnLeftButtonUp();
}

void iASlicerInteractorStyle::OnMouseWheelForward()
{
	if (this->Interactor->GetControlKey() || this->Interactor->GetShiftKey())
	{
		return;
	}
	vtkInteractorStyleImage::OnMouseWheelForward();
}

void iASlicerInteractorStyle::OnMouseWheelBackward()
{
	if (this->Interactor->GetControlKey() || this->Interactor->GetShiftKey())
	{
		return;
	}
	vtkInteractorStyleImage::OnMouseWheelBackward();
}

void iASlicerInteractorStyle::OnRightButtonDown()
{
	if (!m_rightButtonDragZoomEnabled)
	{
		return;
	}
	vtkInteractorStyleImage::OnRightButtonDown();
}
void iASlicerInteractorStyle::setRightButtonDragZoomEnabled(bool enabled)
{
	m_rightButtonDragZoomEnabled = enabled;
}
void iASlicerInteractorStyle::setInteractionMode(InteractionMode mode)
{
	m_interactionMode = mode;
}
bool iASlicerInteractorStyle::leftButtonDown() const
{
	return m_leftButtonDown;
}
iASlicerInteractorStyle::InteractionMode iASlicerInteractorStyle::interactionMode() const
{
	return m_interactionMode;
}

void iASlicerInteractorStyle::updateSelectionRect()
{
	m_selRectPolyData->GetPoints()->SetPoint(0, m_dragStart[0], m_dragStart[1], 0);
	m_selRectPolyData->GetPoints()->SetPoint(1, m_dragStart[0], m_dragEnd[1], 0);
	m_selRectPolyData->GetPoints()->SetPoint(2, m_dragEnd[0], m_dragEnd[1], 0);
	m_selRectPolyData->GetPoints()->SetPoint(3, m_dragEnd[0], m_dragStart[1], 0);
	m_selRectPolyData->GetPoints()->Modified();
}

iASlicerInteractionEvents const& iASlicerInteractorStyle::qtEventObject() const
{
	return m_events;
}
