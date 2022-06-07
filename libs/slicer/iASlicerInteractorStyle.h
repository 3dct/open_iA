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
#pragma once

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>

#include <QObject>

class vtkActor2D;
class vtkPolyData;
class vtkPolyDataMapper2D;

//! Custom interactor style for slicers, disabling some interactions from vtkInteractorStyleImage
//! (e.g. rotation via ctrl+drag, window/level adjustments if not explicitly enabled), and adding
//! a transfer function by region mode.
class iASlicerInteractorStyle : public QObject, public vtkInteractorStyleImage
{
	Q_OBJECT
public:
	enum InteractionMode
	{
		imNormal,
		imRegionSelect,
		imWindowLevelAdjust
	};
	static iASlicerInteractorStyle* New();
	vtkTypeMacro(iASlicerInteractorStyle, vtkInteractorStyleImage);

	iASlicerInteractorStyle();

	void OnLeftButtonDown() override;
	void OnMouseMove() override;
	void OnLeftButtonUp() override;
	//! @{ shift and control + mousewheel are used differently - don't use them for zooming!
	void OnMouseWheelForward() override;
	void OnMouseWheelBackward() override;
	//! @}
	//! @{ Conditionally disable zooming via right button dragging
	void OnRightButtonDown() override;
	void setRightButtonDragZoomEnabled(bool enabled);
	void setInteractionMode(InteractionMode mode);
	bool leftButtonDown() const;
	InteractionMode interactionMode() const;

	//! @}
	/*
	virtual void OnChar()
	{
		vtkRenderWindowInteractor *rwi = this->Interactor;
		switch (rwi->GetKeyCode())
		{ // disable 'picking' action on p
		case 'P':
		case 'p':
			break;
		default:
			vtkInteractorStyleImage::OnChar();
		}
	}
	*/
signals:
	void selection(int dragStart[2], int dragEnd[2]);

private:
	void updateSelectionRect();

	bool m_rightButtonDragZoomEnabled;
	bool m_leftButtonDown;
	InteractionMode m_interactionMode;
	int m_dragStart[2], m_dragEnd[2];

	vtkSmartPointer<vtkPolyData> m_selRectPolyData;
	vtkSmartPointer<vtkPolyDataMapper2D> m_selRectMapper;
	vtkSmartPointer<vtkActor2D> m_selRectActor;
};