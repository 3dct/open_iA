/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#include "pch.h"
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
#include <vtkRenderWindow.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

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
	volProp->SetColor(0, transfer->GetColorFunction());
	volProp->SetScalarOpacity(0, transfer->GetOpacityFunction());
	volMapper->SetBlendModeToComposite();
	volMapper->SetInputData(imgData);
	volume->SetMapper(volMapper);
	volume->SetProperty(volProp);
	volume->SetVisibility(true);

	outlineFilter->SetInputData(imgData);
	outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
	outlineActor->GetProperty()->SetColor(0, 0, 0);
	outlineActor->PickableOff();
	outlineActor->SetMapper(outlineMapper);
}

void iAVolumeRenderer::ApplySettings(iAVolumeSettings const & vs)
{
	volProp->SetAmbient(vs.AmbientLighting);
	volProp->SetDiffuse(vs.DiffuseLighting);
	volProp->SetSpecular(vs.SpecularLighting);
	volProp->SetSpecularPower(vs.SpecularPower);
	volProp->SetInterpolationType(vs.LinearInterpolation);
	volProp->SetShade(vs.Shading);
	volMapper->SetRequestedRenderMode(vs.Mode);
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

void iAVolumeRenderer::AddTo(vtkRenderer* w)
{
	if (currentRenderer)
	{
		if (currentRenderer != w)
		{
			Remove();
		}
		else
		{
			return;
		}
	}
	w->AddVolume(volume);
	currentRenderer = w;
}

void iAVolumeRenderer::Remove()
{
	if (!currentRenderer)
	{
		DEBUG_LOG("RemoveFromWindow called on VolumeRenderer which was not attached to a window!\n");
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
	volume->Update();
	volMapper->Update();
}


void iAVolumeRenderer::SetCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3)
{
	volMapper->AddClippingPlane(p1);
	volMapper->AddClippingPlane(p2);
	volMapper->AddClippingPlane(p3);
}

void iAVolumeRenderer::RemoveCuttingPlanes()
{
	volMapper->RemoveAllClippingPlanes();
}
