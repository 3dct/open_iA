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

//! Separates Qt signal out from iASlicerInteractorStyle, to avoid iASlicerInteractorStyle being moc'ed
//! directly; the mix of deriving from QObject and the Q_OBJECT declaration with the vtk way of object
//! inheritance causes problems, a least with Qt 6:
//! qmetatype.h(2250,1): error C2660: 'vtkObject::operator new': function does not take 3 arguments
class iASlicerInteractionEvents: public QObject
{
	Q_OBJECT

	friend class iASlicerInteractorStyle;
signals:
	void selection(int dragStart[2], int dragEnd[2]) const;
	void sliceChange(int direction) const;

private:
	void triggerSelection(int dragStart[2], int dragEnd[2]);
	void triggerSliceChange(int direction);
};

//! Custom interactor style for slicers, disabling some interactions from vtkInteractorStyleImage
//! (e.g. rotation via ctrl+drag, window/level adjustments if not explicitly enabled), and adding
//! a transfer function by region mode.
class iASlicerInteractorStyle: public vtkInteractorStyleImage
{
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
	
	iASlicerInteractionEvents const & qtEventObject() const;

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

private:
	void updateSelectionRect();

	bool m_rightButtonDragZoomEnabled;
	bool m_leftButtonDown;
	InteractionMode m_interactionMode;
	int m_dragStart[2], m_dragEnd[2];
	iASlicerInteractionEvents m_events;

	vtkSmartPointer<vtkPolyData> m_selRectPolyData;
	vtkSmartPointer<vtkPolyDataMapper2D> m_selRectMapper;
	vtkSmartPointer<vtkActor2D> m_selRectActor;
};