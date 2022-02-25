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
#include "iA3DPolyObjectActor.h"

#include <iA3DColoredPolyObjectVis.h>

#include <vtkActor.h>
#include <vtkCommand.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

class iARenderDeleteListener : public vtkCommand
{
public:
	static iARenderDeleteListener* New()
	{
		return new iARenderDeleteListener();
	}
	void setObjActor(iA3DObjectActor* objActor)
	{
		m_objActor = objActor;
	}
	void Execute(vtkObject*, unsigned long, void*) override
	{
		m_objActor->clearRenderer();;
	}
private:
	iA3DObjectActor* m_objActor;
};


iA3DPolyObjectActor::iA3DPolyObjectActor(vtkRenderer* ren, iA3DColoredPolyObjectVis* obj) :
	iA3DObjectActor(ren),
	m_visible(false),
	m_clippingPlanesEnabled(false),
	m_simple(false),
	m_outlineVisible(false),
	m_obj(obj),
	m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_actor(vtkSmartPointer<vtkActor>::New()),
	m_outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	m_outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_outlineActor(vtkSmartPointer<vtkActor>::New()),
	m_renderDeleteListener(vtkSmartPointer<iARenderDeleteListener>::New())
{
	if (obj->finalPolyData())
	{
		m_polyData = obj->finalPolyData();
	}
	else
	{
		m_polyData = obj->polyData();
	}
	m_mapper->SetInputData(m_polyData);
	m_outlineFilter->SetInputData(m_polyData);
	m_mapper->SetScalarModeToUsePointFieldData();
	m_mapper->ScalarVisibilityOn();
	m_mapper->SelectColorArray("Colors");
	m_actor->SetMapper(m_mapper);

	m_outlineMapper->SetInputConnection(m_outlineFilter->GetOutputPort());
	m_outlineActor->GetProperty()->SetColor(0, 0, 0);
	m_outlineActor->PickableOff();
	m_outlineActor->SetMapper(m_outlineMapper);

	m_renderDeleteListener->setObjActor(this);
	m_renObserverTag = m_ren->AddObserver(vtkCommand::DeleteEvent, m_renderDeleteListener);
}

iA3DPolyObjectActor::~iA3DPolyObjectActor()
{
	if (m_ren)
	{
		m_ren->RemoveObserver(m_renObserverTag);
	}
	hide();
}

void iA3DPolyObjectActor::show()
{
	if (m_visible || !m_ren)
	{
		return;
	}
	m_ren->AddActor(m_actor);
	m_visible = true;
}

void iA3DPolyObjectActor::hide()
{
	if (!m_visible || !m_ren)
	{
		return;
	}
	m_ren->RemoveActor(m_actor);
	m_visible = false;
}

void iA3DPolyObjectActor::updateRenderer()
{
	if (m_visible || m_outlineVisible)
	{
		iA3DObjectActor::updateRenderer();
	}
}

void iA3DPolyObjectActor::setShowWireFrame(bool show)
{
	if (show)
	{
		m_actor->GetProperty()->SetRepresentationToWireframe();
	}
	else
	{
		m_actor->GetProperty()->SetRepresentationToSurface();
	}
	updateMapper();
}

void iA3DPolyObjectActor::updateMapper()
{
	m_mapper->Update();
	updateRenderer();
}

vtkActor* iA3DPolyObjectActor::actor()
{
	return m_actor;
}

void iA3DPolyObjectActor::showBoundingBox()
{
	if (m_outlineVisible || !m_ren)
	{
		return;
	}
	m_outlineMapper->Update();
	m_ren->AddActor(m_outlineActor);
	m_outlineVisible = true;
	updateRenderer();
}

void iA3DPolyObjectActor::hideBoundingBox()
{
	if (!m_outlineVisible || !m_ren)
	{
		return;
	}
	m_ren->RemoveActor(m_outlineActor);
	m_outlineVisible = false;
	updateRenderer();
}

void iA3DPolyObjectActor::setShowSimple(bool simple)
{
	m_simple = simple;
	if (m_simple)
	{
		m_mapper->SetInputData(m_obj->polyData());
	}
	else
	{
		m_mapper->SetInputData(m_obj->finalPolyData());
	}
}

void iA3DPolyObjectActor::setClippingPlanes(vtkPlane* planes[3])
{
	if (m_clippingPlanesEnabled)
	{
		return;
	}
	m_clippingPlanesEnabled = true;
	for (int i = 0; i < 3; ++i)
	{
		m_mapper->AddClippingPlane(planes[i]);
	}
}

void iA3DPolyObjectActor::removeClippingPlanes()
{
	m_mapper->RemoveAllClippingPlanes();
	m_clippingPlanesEnabled = false;
}


bool iA3DPolyObjectActor::visible() const
{
	return m_visible;
}
