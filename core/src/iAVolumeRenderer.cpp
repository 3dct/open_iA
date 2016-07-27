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
	renderer(vtkSmartPointer<vtkOpenGLRenderer>::New()),
	volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
	outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	outlineActor(vtkSmartPointer<vtkActor>::New()),
	outlineRenderer(vtkSmartPointer<vtkOpenGLRenderer>::New()),
	currentWindow(0)
{
	volProp->SetColor(0, transfer->GetColorFunction());
	volProp->SetScalarOpacity(0, transfer->GetOpacityFunction());
	volMapper->SetBlendModeToComposite();
	volMapper->SetInputData(imgData);
	volume->SetMapper(volMapper);
	volume->SetProperty(volProp);
	volume->SetVisibility(true);
	renderer->AddVolume(volume);

	outlineFilter->SetInputData(imgData);
	outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
	outlineActor->GetProperty()->SetColor(0, 0, 0);
	outlineActor->PickableOff();
	outlineActor->SetMapper(outlineMapper);
	outlineRenderer->AddActor(outlineActor);
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
	volMapper->InteractiveAdjustSampleDistancesOff();
	volMapper->SetSampleDistance(vs.SampleDistance);
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

void iAVolumeRenderer::AddToWindow(vtkRenderWindow* w)
{
	if (currentWindow)
	{
		if (currentWindow != w)
		{
			RemoveFromWindow();
		}
		else
		{
			return;
		}
	}
	w->AddRenderer(renderer);
	vtkCamera* cam = w->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	renderer->SetActiveCamera(cam);
	renderer->SetLayer(2);
	renderer->SetBackground(1, 0.5, 0.5);
	renderer->InteractiveOn();
	currentWindow = w;
}

void iAVolumeRenderer::AddBoundingBoxToWindow(vtkRenderWindow* w)
{
	if (currentBoundingBoxWindow)
	{
		if (currentBoundingBoxWindow != w)
		{
			RemoveBoundingBoxFromWindow();
		}
		else
		{
			return;
		}
	}
	outlineRenderer->SetLayer(3);
	outlineRenderer->InteractiveOff();
	vtkCamera* cam = w->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	outlineRenderer->SetActiveCamera(cam);
	w->AddRenderer(outlineRenderer);
	currentBoundingBoxWindow = w;
}


void iAVolumeRenderer::RemoveBoundingBoxFromWindow()
{
	if (!currentBoundingBoxWindow)
		return;
	currentBoundingBoxWindow->RemoveRenderer(outlineRenderer);
	currentBoundingBoxWindow = 0;
}

void iAVolumeRenderer::UpdateBoundingBoxPosition()
{
	if (!currentBoundingBoxWindow)
		return;
	outlineActor->SetOrientation(volume->GetOrientation());
	outlineActor->SetPosition(volume->GetPosition());
}

void iAVolumeRenderer::RemoveFromWindow()
{
	if (!currentWindow)
	{
		DEBUG_LOG("RemoveFromWindow called on VolumeRenderer which was not attached to a window!\n");
		return;
	}
	currentWindow->RemoveRenderer(renderer);
	currentWindow = 0;
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
