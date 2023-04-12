// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>

#include <QObject>

class vtkActor2D;
class vtkPolyData;
class vtkPolyDataMapper2D;

//! Separates Qt signals from iASlicerInteractorStyle.
//! This avoids iASlicerInteractorStyle being moc'ed directly; the mix of deriving from QObject and the
//! Q_OBJECT declaration with the vtk way of object inheritance causes problems, a least with Qt 6:
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

//! Custom interactor style for slicers, changing some interactions from vtkInteractorStyleImage.
//! Changed interactions:
//! <ul>
//!   <li>Disables rotation via ctrl+drag</li>
//!   <li>Disables window/level adjustments (unless explicitly enabled)</li>
//!   <li>Option to disables right mouse button zooming (e.g. when context menu is used)</li>
//!   <li>Adds region selection (used to create a transfer function with optimal contrast for the selected region)</li>
//! </ul>
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
