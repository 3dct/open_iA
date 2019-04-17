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
	volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
	volume(vtkSmartPointer<vtkVolume>::New()),
	volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
	outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	outlineActor(vtkSmartPointer<vtkActor>::New()),
	currentRenderer(0),
	currentBoundingBoxRenderer(0)
{
	m_isFlat = IsFlat(imgData->GetExtent());
	if (!m_isFlat)
	{
		volMapper->SetBlendModeToComposite();
		volume->SetMapper(volMapper);
		volume->SetProperty(volProp);
		volume->SetVisibility(true);
		outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
		outlineActor->GetProperty()->SetColor(0, 0, 0);
		outlineActor->PickableOff();
		outlineActor->SetMapper(outlineMapper);

		SetImage(transfer, imgData);
	}
}


void iAVolumeRenderer::SetImage(iATransferFunction * transfer, vtkSmartPointer<vtkImageData> imgData)
{
	m_isFlat = IsFlat(imgData->GetExtent());
	if (m_isFlat)
		return;
	volMapper->SetInputData(imgData);
	if ( imgData->GetNumberOfScalarComponents() > 1 )
	{
		volMapper->SetBlendModeToComposite();
		volProp->SetIndependentComponents( 0 );
	}
	else
	{
		if (m_VolSettings.ScalarOpacityUnitDistance < 0)
		{
			m_VolSettings.ScalarOpacityUnitDistance = imgData->GetSpacing()[0];
			volProp->SetScalarOpacityUnitDistance(imgData->GetSpacing()[0]);
		}
		volProp->SetColor(0, transfer->getColorFunction());
		volProp->SetScalarOpacity(0, transfer->getOpacityFunction());
	}
	volProp->Modified();
	outlineFilter->SetInputData(imgData);
	Update();
}

void iAVolumeRenderer::SetMovable(bool movable)
{
	volume->SetPickable(movable);
	volume->SetDragable(movable);
}

const iAVolumeSettings& iAVolumeRenderer::getVolumeSettings() const
{
	return m_VolSettings;
}

void iAVolumeRenderer::ApplySettings(iAVolumeSettings const & vs)
{
	if (m_isFlat)
		return;
	m_VolSettings = vs;
	volProp->SetAmbient(vs.AmbientLighting);
	volProp->SetDiffuse(vs.DiffuseLighting);
	volProp->SetSpecular(vs.SpecularLighting);
	volProp->SetSpecularPower(vs.SpecularPower);
	volProp->SetInterpolationType(vs.LinearInterpolation);
	volProp->SetShade(vs.Shading);
	if (vs.ScalarOpacityUnitDistance > 0)
		volProp->SetScalarOpacityUnitDistance(vs.ScalarOpacityUnitDistance);
	else
		m_VolSettings.ScalarOpacityUnitDistance = volProp->GetScalarOpacityUnitDistance();
	volMapper->SetRequestedRenderMode(vs.RenderMode);
#ifdef VTK_OPENGL2_BACKEND
	volMapper->SetSampleDistance(vs.SampleDistance);
	volMapper->InteractiveAdjustSampleDistancesOff();
#endif
}

double * iAVolumeRenderer::GetOrientation()
{
	return volume->GetOrientation();
}

double * iAVolumeRenderer::GetPosition()
{
	return volume->GetPosition();
}

void iAVolumeRenderer::SetOrientation(double* orientation)
{
	volume->SetOrientation(orientation);
	outlineActor->SetOrientation(orientation);
}

void iAVolumeRenderer::SetPosition(double* position)
{
	volume->SetPosition(position);
	outlineActor->SetPosition(position);
}

void iAVolumeRenderer::AddTo(vtkRenderer* r)
{
	if (currentRenderer)
	{
		if (currentRenderer != r)
		{
			Remove();
		}
		else
		{
			return;
		}
	}
	if (m_isFlat)
		return;
	r->AddVolume(volume);
	currentRenderer = r;
}

void iAVolumeRenderer::Remove()
{
	if (!currentRenderer)
	{
		if (!m_isFlat)
			DEBUG_LOG("RemoveFromWindow called on VolumeRenderer which was not attached to a window!");
		return;
	}
	currentRenderer->RemoveVolume(volume);
	currentRenderer = 0;
}

void iAVolumeRenderer::AddBoundingBoxTo(vtkRenderer* w)
{
	if (currentBoundingBoxRenderer)
	{
		if (currentBoundingBoxRenderer != w)
		{
			RemoveBoundingBox();
		}
		else
		{
			return;
		}
	}
	w->AddActor(outlineActor);
	currentBoundingBoxRenderer = w;
}


void iAVolumeRenderer::RemoveBoundingBox()
{
	if (!currentBoundingBoxRenderer)
		return;
	currentBoundingBoxRenderer->RemoveActor(outlineActor);
	currentBoundingBoxRenderer = 0;
}

void iAVolumeRenderer::UpdateBoundingBox()
{
	if (!currentBoundingBoxRenderer)
		return;
	outlineActor->SetOrientation(volume->GetOrientation());
	outlineActor->SetPosition(volume->GetPosition());
}


vtkSmartPointer<vtkVolume> iAVolumeRenderer::GetVolume()
{
	return volume;
}

void iAVolumeRenderer::Update()
{
	if (m_isFlat)
		return;
	volume->Update();
	volMapper->Update();
	outlineMapper->Update();
}


void iAVolumeRenderer::SetCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3)
{
	if (m_isFlat)
		return;
	volMapper->AddClippingPlane(p1);
	volMapper->AddClippingPlane(p2);
	volMapper->AddClippingPlane(p3);
}

void iAVolumeRenderer::RemoveCuttingPlanes()
{
	if (m_isFlat)
		return;
	volMapper->RemoveAllClippingPlanes();
}


void iAVolumeRenderer::ShowVolume(bool visible)
{
	if (m_isFlat)
		return;
	volume->SetVisibility(visible);
}

void iAVolumeRenderer::ShowBoundingBox(bool visible)
{
	outlineActor->SetVisibility(visible);
}
