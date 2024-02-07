// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaobjectvis_export.h"

#include "iAObjectVisActor.h"

#include <vtkSmartPointer.h>

class iARenderDeleteListener;
class iAColoredPolyObjectVis;

class vtkActor;
class vtkOutlineFilter;
class vtkPlane;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderer;

//! Displays data from objects in a class derived from iAColoredPolyObjectVis
class iAobjectvis_API iAPolyObjectVisActor: public iAObjectVisActor
{
public:
	//! create a new visualization of the given 3D object in the given renderer
	//! @param ren the VTK renderer to which the 3D object will be added
	//! @param obj the polydata object that is dislayed
	iAPolyObjectVisActor(vtkRenderer* ren, iAColoredPolyObjectVis* obj);
	~iAPolyObjectVisActor();
	//! show the object
	void show() override;
	//! hide the object
	void hide() override;
	//! whether the object is currently shown
	bool visible() const;
	//! show the object's bounding box
	void showBoundingBox();
	//! hide the object's bounding box
	void hideBoundingBox();
	//! switch between "simplest" vis (e.g. lines) and default complex vis (e.g. cylinder)
	void setShowSimple(bool simple);
	//! switch between "normal" surface display mode and wireframe display
	void setShowWireFrame(bool show);
	//! set given planes as clipping planes for the viewed object
	void setClippingPlanes(vtkPlane* planes[3]);
	//! remove any clipping planes
	void removeClippingPlanes();
	//! retrieve actor that contains object (think of removing this method!)
	vtkActor* actor();
	//! update the renderer displaying the object.
	void updateRenderer() override;
	//! Triggers an update of the color mapper and the renderer.
	void updateMapper();

private:
	bool m_visible, m_clippingPlanesEnabled, m_simple, m_outlineVisible;
	iAColoredPolyObjectVis* m_obj;

	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
	vtkSmartPointer<vtkActor> m_actor;

	vtkSmartPointer<vtkOutlineFilter> m_outlineFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_outlineMapper;
	vtkSmartPointer<vtkActor> m_outlineActor;

	vtkSmartPointer<vtkPolyData> m_polyData;
	unsigned long m_renObserverTag;
	vtkSmartPointer<iARenderDeleteListener> m_renderDeleteListener;
};
