/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAVolumeRenderer.h"

#include "iAConsole.h"
#include "iATransferFunction.h"
#include "iAVolumeSettings.h"

#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRendererCollection.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

bool IsFlat(int extent[6])
{
	return extent[0] == extent[1] ||
		extent[2] == extent[3] ||
		extent[4] == extent[5];
}

iAVolumeRenderer::iAVolumeRenderer(
	iATransferFunction * transfer,
	vtkSmartPointer<vtkImageData> imgData)
:
	m_volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
	m_volume(vtkSmartPointer<vtkVolume>::New()),
	m_volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
	m_outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	m_outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_outlineActor(vtkSmartPointer<vtkActor>::New()),
	m_currentRenderer(0),
	m_currentBoundingBoxRenderer(0)
{
	m_isFlat = IsFlat(imgData->GetExtent());
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
	m_isFlat = IsFlat(imgData->GetExtent());
	if (m_isFlat)
		return;
	m_volMapper->SetInputData(imgData);
	if ( imgData->GetNumberOfScalarComponents() > 1 )
	{
		m_volMapper->SetBlendModeToComposite();
		m_volProp->SetIndependentComponents( 0 );
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

const iAVolumeSettings& iAVolumeRenderer::volumeSettings() const
{
	return m_volSettings;
}

void iAVolumeRenderer::applySettings(iAVolumeSettings const & vs)
{
	if (m_isFlat)
		return;
	m_volSettings = vs;
	m_volProp->SetAmbient(vs.AmbientLighting);
	m_volProp->SetDiffuse(vs.DiffuseLighting);
	m_volProp->SetSpecular(vs.SpecularLighting);
	m_volProp->SetSpecularPower(vs.SpecularPower);
	m_volProp->SetInterpolationType(vs.LinearInterpolation);
	m_volProp->SetShade(vs.Shading);
	if (vs.ScalarOpacityUnitDistance > 0)
		m_volProp->SetScalarOpacityUnitDistance(vs.ScalarOpacityUnitDistance);
	else
		m_volSettings.ScalarOpacityUnitDistance = m_volProp->GetScalarOpacityUnitDistance();
	m_volMapper->SetRequestedRenderMode(vs.RenderMode);
#ifdef VTK_OPENGL2_BACKEND
	m_volMapper->SetSampleDistance(vs.SampleDistance);
	m_volMapper->InteractiveAdjustSampleDistancesOff();
#endif
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
		return;
	r->AddVolume(m_volume);
	m_currentRenderer = r;
}

void iAVolumeRenderer::remove()
{
	if (!m_currentRenderer)
	{
		if (!m_isFlat)
			DEBUG_LOG("RemoveFromWindow called on VolumeRenderer which was not attached to a window!");
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
		return;
	m_currentBoundingBoxRenderer->RemoveActor(m_outlineActor);
	m_currentBoundingBoxRenderer = nullptr;
}

void iAVolumeRenderer::updateBoundingBox()
{
	if (!m_currentBoundingBoxRenderer)
		return;
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
		return;
	m_volume->Modified();
	m_volume->Update();
	m_volMapper->Modified();
	m_volMapper->Update();
	m_outlineMapper->Update();
}


void iAVolumeRenderer::setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3)
{
	if (m_isFlat)
		return;
	m_volMapper->AddClippingPlane(p1);
	m_volMapper->AddClippingPlane(p2);
	m_volMapper->AddClippingPlane(p3);
}

void iAVolumeRenderer::removeCuttingPlanes()
{
	if (m_isFlat)
		return;
	m_volMapper->RemoveAllClippingPlanes();
}


void iAVolumeRenderer::showVolume(bool visible)
{
	if (m_isFlat)
		return;
	m_volume->SetVisibility(visible);
}

void iAVolumeRenderer::showBoundingBox(bool visible)
{
	m_outlineActor->SetVisibility(visible);
}
