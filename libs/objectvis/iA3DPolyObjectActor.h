/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAobjectvis_export.h"

#include "iA3DObjectActor.h"

#include <vtkSmartPointer.h>

class iARenderDeleteListener;
class iA3DColoredPolyObjectVis;

class vtkActor;
class vtkOutlineFilter;
class vtkPlane;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderer;

//! Displays data from objects in a class derived from iA3DColoredPolyObjectVis
class iAobjectvis_API iA3DPolyObjectActor : public iA3DObjectActor
{
public:
	//! create a new visualization of the given 3D object in the given renderer
	//! @param ren the VTK renderer to which the 3D object will be added
	//! @param obj the polydata object that is dislayed
	iA3DPolyObjectActor(vtkRenderer* ren, iA3DColoredPolyObjectVis* obj);
	~iA3DPolyObjectActor();
	//! show the object
	void show() override;
	//! hide the object
	void hide();
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
	iA3DColoredPolyObjectVis* m_obj;

	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
	vtkSmartPointer<vtkActor> m_actor;

	vtkSmartPointer<vtkOutlineFilter> m_outlineFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_outlineMapper;
	vtkSmartPointer<vtkActor> m_outlineActor;

	vtkSmartPointer<vtkPolyData> m_polyData;
	unsigned long m_renObserverTag;
	vtkSmartPointer<iARenderDeleteListener> m_renderDeleteListener;
};
