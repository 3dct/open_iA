// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVolumeRenderer.h"

#include "iALog.h"
#include "iAToolsVTK.h"    // for isFlat
#include "iATransferFunction.h"
#include "iAVolumeSettings.h"

#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkProperty.h>

iAVolumeRenderer::iAVolumeRenderer(
	iATransferFunction * transfer,
	vtkSmartPointer<vtkImageData> imgData)
:
	m_volume(vtkSmartPointer<vtkVolume>::New()),
	m_volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
	m_volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
	m_currentRenderer(nullptr),
	m_outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	m_outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_outlineActor(vtkSmartPointer<vtkActor>::New()),
	m_currentBoundingBoxRenderer(nullptr)
{
	m_isFlat = isFlat(imgData);
	if (!m_isFlat)
	{
		m_volMapper->SetBlendModeToComposite();
		m_volume->SetMapper(m_volMapper);
		m_volume->SetProperty(m_volProp);
		m_volume->SetVisibility(true);
		m_outlineMapper->SetInputConnection(m_outlineFilter->GetOutputPort());
		m_outlineActor->GetProperty()->SetColor(0, 0, 0);
		m_outlineActor->PickableOff();
		m_outlineActor->SetMapper(m_outlineMapper);

		setImage(transfer, imgData);
	}
}


void iAVolumeRenderer::setImage(iATransferFunction * transfer, vtkSmartPointer<vtkImageData> imgData)
{
	m_isFlat = isFlat(imgData);
	if (m_isFlat)
	{
		return;
	}
	m_volMapper->SetInputData(imgData);
	if ( imgData->GetNumberOfScalarComponents() > 1 )
	{
		m_volMapper->SetBlendModeToComposite();
		m_volProp->IndependentComponentsOff();
		m_volProp->SetScalarOpacity(0, transfer->opacityTF());
	}
	else
	{
		if (m_volSettings.ScalarOpacityUnitDistance < 0)
		{
			m_volSettings.ScalarOpacityUnitDistance = imgData->GetSpacing()[0];
			m_volProp->SetScalarOpacityUnitDistance(imgData->GetSpacing()[0]);
		}
		m_volProp->SetColor(0, transfer->colorTF());
		m_volProp->SetScalarOpacity(0, transfer->opacityTF());
	}
	m_volProp->Modified();
	m_outlineFilter->SetInputData(imgData);
	update();
}

void iAVolumeRenderer::setMovable(bool movable)
{
	m_volume->SetPickable(movable);
	m_volume->SetDragable(movable);
}

bool iAVolumeRenderer::isRendered() const
{
	return m_currentRenderer;
}

bool iAVolumeRenderer::isVisible() const
{
	return isRendered() && m_volume->GetVisibility();
}

const iAVolumeSettings& iAVolumeRenderer::volumeSettings() const
{
	return m_volSettings;
}

void iAVolumeRenderer::applySettings(iAVolumeSettings const & vs)
{
	if (m_isFlat)
	{
		return;
	}
	m_volSettings = vs;
	m_volProp->SetAmbient(vs.AmbientLighting);
	m_volProp->SetDiffuse(vs.DiffuseLighting);
	m_volProp->SetSpecular(vs.SpecularLighting);
	m_volProp->SetSpecularPower(vs.SpecularPower);
	m_volProp->SetInterpolationType(vs.LinearInterpolation);
	m_volProp->SetShade(vs.Shading);
	if (vs.ScalarOpacityUnitDistance > 0)
	{
		m_volProp->SetScalarOpacityUnitDistance(vs.ScalarOpacityUnitDistance);
	}
	else
	{
		m_volSettings.ScalarOpacityUnitDistance = m_volProp->GetScalarOpacityUnitDistance();
	}
	m_volMapper->SetRequestedRenderMode(vs.RenderMode);
	m_volMapper->SetSampleDistance(vs.SampleDistance);
	m_volMapper->InteractiveAdjustSampleDistancesOff();
}

double const * iAVolumeRenderer::orientation() const
{
	return m_volume->GetOrientation();
}

double const * iAVolumeRenderer::position() const
{
	return m_volume->GetPosition();
}

void iAVolumeRenderer::setOrientation(double* orientation)
{
	m_volume->SetOrientation(orientation);
	m_outlineActor->SetOrientation(orientation);
}

void iAVolumeRenderer::setPosition(double* position)
{
	m_volume->SetPosition(position);
	m_outlineActor->SetPosition(position);
}

void iAVolumeRenderer::addTo(vtkRenderer* r)
{
	if (m_currentRenderer)
	{
		if (m_currentRenderer != r)
		{
			remove();
		}
		else
		{
			return;
		}
	}
	if (m_isFlat)
	{
		return;
	}
	r->AddVolume(m_volume);
	m_currentRenderer = r;
}

void iAVolumeRenderer::remove()
{
	if (!m_currentRenderer)
	{
		if (!m_isFlat)
		{
			LOG(lvlWarn, "RemoveFromWindow called on VolumeRenderer which was not attached to a window!");
		}
		return;
	}
	m_currentRenderer->RemoveVolume(m_volume);
	m_currentRenderer = nullptr;
}

void iAVolumeRenderer::addBoundingBoxTo(vtkRenderer* w)
{
	if (m_currentBoundingBoxRenderer)
	{
		if (m_currentBoundingBoxRenderer != w)
		{
			removeBoundingBox();
		}
		else
		{
			return;
		}
	}
	w->AddActor(m_outlineActor);
	m_currentBoundingBoxRenderer = w;
}


void iAVolumeRenderer::removeBoundingBox()
{
	if (!m_currentBoundingBoxRenderer)
	{
		return;
	}
	m_currentBoundingBoxRenderer->RemoveActor(m_outlineActor);
	m_currentBoundingBoxRenderer = nullptr;
}

void iAVolumeRenderer::updateBoundingBox()
{
	if (!m_currentBoundingBoxRenderer)
	{
		return;
	}
	m_outlineActor->SetOrientation(m_volume->GetOrientation());
	m_outlineActor->SetPosition(m_volume->GetPosition());
}

vtkSmartPointer<vtkVolume> iAVolumeRenderer::volume()
{
	return m_volume;
}

vtkRenderer* iAVolumeRenderer::currentRenderer()
{
	return m_currentRenderer;
}

void iAVolumeRenderer::update()
{
	if (m_isFlat)
	{
		return;
	}
	m_volume->Modified();
	m_volume->Update();
	m_volMapper->Modified();
	m_volMapper->Update();
	m_outlineMapper->Update();
}


void iAVolumeRenderer::setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3)
{
	if (m_isFlat)
	{
		return;
	}
	m_volMapper->AddClippingPlane(p1);
	m_volMapper->AddClippingPlane(p2);
	m_volMapper->AddClippingPlane(p3);
}

void iAVolumeRenderer::removeCuttingPlanes()
{
	if (m_isFlat)
	{
		return;
	}
	m_volMapper->RemoveAllClippingPlanes();
}


void iAVolumeRenderer::showVolume(bool visible)
{
	if (m_isFlat)
	{
		return;
	}
	m_volume->SetVisibility(visible);
}

void iAVolumeRenderer::showBoundingBox(bool visible)
{
	m_outlineActor->SetVisibility(visible);
}
