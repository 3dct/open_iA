// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPolyObjectVisActor.h"

#include <iAColoredPolyObjectVis.h>

#include <vtkActor.h>
#include <vtkCommand.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

//! Listener class helping to prevent access to deleted renderers.
//! Used in iAPolyObjectVisActor to detect when the renderer this actor is attached to
//! gets deleted. In that case, the internal renderer pointer is set to nullptr to
//! avoid an invalid access to the deleted renderer.
class iARenderDeleteListener : public vtkCommand
{
public:
	static iARenderDeleteListener* New()
	{
		return new iARenderDeleteListener();
	}
	void setObjActor(iAObjectVisActor* objActor)
	{
		m_objActor = objActor;
	}
	void Execute(vtkObject*, unsigned long, void*) override
	{
		m_objActor->clearRenderer();
	}
private:
	iAObjectVisActor* m_objActor;
};


iAPolyObjectVisActor::iAPolyObjectVisActor(vtkRenderer* ren, iAColoredPolyObjectVis* obj) :
	iAObjectVisActor(ren),
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

iAPolyObjectVisActor::~iAPolyObjectVisActor()
{
	if (m_ren)
	{
		m_ren->RemoveObserver(m_renObserverTag);
	}
	hide();
}

void iAPolyObjectVisActor::show()
{
	if (m_visible || !m_ren)
	{
		return;
	}
	m_ren->AddActor(m_actor);
	m_visible = true;
}

void iAPolyObjectVisActor::hide()
{
	if (!m_visible || !m_ren)
	{
		return;
	}
	m_ren->RemoveActor(m_actor);
	m_visible = false;
}

void iAPolyObjectVisActor::updateRenderer()
{
	if (m_visible || m_outlineVisible)
	{
		iAObjectVisActor::updateRenderer();
	}
}

void iAPolyObjectVisActor::setShowWireFrame(bool show)
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

void iAPolyObjectVisActor::updateMapper()
{
	m_mapper->Update();
	updateRenderer();
}

vtkActor* iAPolyObjectVisActor::actor()
{
	return m_actor;
}

void iAPolyObjectVisActor::showBoundingBox()
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

void iAPolyObjectVisActor::hideBoundingBox()
{
	if (!m_outlineVisible || !m_ren)
	{
		return;
	}
	m_ren->RemoveActor(m_outlineActor);
	m_outlineVisible = false;
	updateRenderer();
}

void iAPolyObjectVisActor::setShowSimple(bool simple)
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

void iAPolyObjectVisActor::setClippingPlanes(vtkPlane* planes[3])
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

void iAPolyObjectVisActor::removeClippingPlanes()
{
	m_mapper->RemoveAllClippingPlanes();
	m_clippingPlanesEnabled = false;
}


bool iAPolyObjectVisActor::visible() const
{
	return m_visible;
}
