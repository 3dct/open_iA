#include "iAPolyObjectActor.h"

#include <vtkPolyDataMapper.h>

iAPolyObjectActor::iAPolyObjectActor(vtkRenderer* ren, QSharedPointer<iA3DColoredPolyObjectVis> obj) :
	m_visible(false),
	m_clippingPlanesEnabled(false),
	m_ren(ren),
	m_obj(obj),
	m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_actor(vtkSmartPointer<vtkActor>::New()),

	m_outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	m_outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_outlineActor(vtkSmartPointer<vtkActor>::New()),
{
	if (obj->finalOutput())
	{
		m_mapper->setInputConnection()
	}
	m_mapper->SetScalarModeToUsePointFieldData();
	m_mapper->ScalarVisibilityOn();
	m_mapper->SelectColorArray("Colors");
	m_actor->SetMapper(m_mapper);
}


void iA3DColoredPolyObjectVis::show()
{
	if (m_visible)
	{
		return;
	}
	m_ren->AddActor(m_actor);
	m_visible = true;
}

void iA3DColoredPolyObjectVis::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_ren->RemoveActor(m_actor);
	m_visible = false;
}

void iA3DColoredPolyObjectVis::updateRenderer()
{
	if (m_visible)
	{
		iA3DObjectVis::updateRenderer();
	}
}

void iA3DColoredPolyObjectVis::setShowWireFrame(bool show)
{
	if (show)
	{
		m_actor->GetProperty()->SetRepresentationToWireframe();
	}
	else
	{
		m_actor->GetProperty()->SetRepresentationToSurface();
	}
	updatePolyMapper();
}

void iA3DColoredPolyObjectVis::updatePolyMapper()
{
	m_colors->Modified();
	m_mapper->Update();
	updateRenderer();
}


vtkSmartPointer<vtkActor> iA3DColoredPolyObjectVis::getActor()
{
	return m_actor;
}

void iA3DColoredPolyObjectVis::setupBoundingBox()
{
	m_outlineFilter->SetInputData(getPolyData());
	m_outlineMapper->SetInputConnection(m_outlineFilter->GetOutputPort());
	m_outlineActor->GetProperty()->SetColor(0, 0, 0);
	m_outlineActor->PickableOff();
	m_outlineActor->SetMapper(m_outlineMapper);
}

void iA3DColoredPolyObjectVis::showBoundingBox()
{
	m_outlineMapper->Update();
	m_ren->AddActor(m_outlineActor);
	updateRenderer();
}

void iA3DColoredPolyObjectVis::hideBoundingBox()
{
	m_ren->RemoveActor(m_outlineActor);
	updateRenderer();
}

void iA3DColoredPolyObjectVis::setClippingPlanes(vtkPlane* planes[3])
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

void iA3DColoredPolyObjectVis::removeClippingPlanes()
{
	m_mapper->RemoveAllClippingPlanes();
	m_clippingPlanesEnabled = false;
}


bool iA3DColoredPolyObjectVis::visible() const
{
	return m_visible;
}


virtual void updateRenderer();

void iA3DObjectVis::updateRenderer()
{
	m_ren->Render();
	emit updated();
}

void iA3DObjectVis::show()
{}
