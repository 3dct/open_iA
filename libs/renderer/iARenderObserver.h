// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iarenderer_export.h"

#include <vtkCommand.h>
#include <vtkSmartPointer.h>

#include <array>
#include <vector>

class vtkPlane;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkWorldPointPicker;

//! Observes the mouse movements in an iARenderer.
//! This class servers the iARenderer class to observe mouse movement and to extract coordinates
//! and the corresponding data "below" the mouse pointer.
class iArenderer_API iARenderObserver: public vtkCommand
{
public:
	iARenderObserver(vtkRenderer* pRen, vtkRenderWindowInteractor* pIren, std::array<vtkPlane*, 3> planes);
	void AddListener(vtkCommand* listener);
	vtkRenderWindowInteractor* GetInteractor();
	vtkWorldPointPicker* GetWorldPicker();
	void PickWithWorldPicker();

protected:
	vtkRenderer* m_pRen;
	vtkRenderWindowInteractor* m_pIren;
	vtkSmartPointer<vtkWorldPointPicker> m_pWorldPicker;

private:
	iARenderObserver(iARenderObserver const& other) = delete;
	iARenderObserver& operator=(iARenderObserver const& other) = delete;
	std::array<vtkPlane*, 3> m_planes;
	std::vector<vtkCommand*> m_listener;

	void Execute(vtkObject *caller, unsigned long, void*) override;
};

//! retrieve the normal vector for the given slicer mode
std::vector<double> slicerNormal(int mode);
