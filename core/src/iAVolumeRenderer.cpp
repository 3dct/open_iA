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

#include "iAModalityTransfer.h"
#include "iAVolumeSettings.h"

#include <vtkImageData.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

iAVolumeRenderer::iAVolumeRenderer(
	QSharedPointer<ModalityTransfer> transfer,
	vtkSmartPointer<vtkImageData> imgData)
{
	double rangeMin = imgData->GetScalarRange()[0];
	double rangeMax = imgData->GetScalarRange()[1];

	volProp = vtkSmartPointer<vtkVolumeProperty>::New();
	volProp->SetColor(transfer->getColorFunction());
	volProp->SetScalarOpacity(transfer->getOpacityFunction());
	CreateVolumeMapper(imgData);
	volume = vtkSmartPointer<vtkVolume>::New();
	volume->SetMapper(volMapper);
	volume->SetProperty(volProp);
	volume->SetVisibility(true);
}

void iAVolumeRenderer::SetRenderSettings(iAVolumeSettings const & rs)
{
	volProp->SetAmbient(rs.AmbientLighting);
	volProp->SetDiffuse(rs.DiffuseLighting);
	volProp->SetSpecular(rs.SpecularLighting);
	volProp->SetSpecularPower(rs.SpecularPower);
	volProp->SetInterpolationType(rs.LinearInterpolation);
	volProp->SetShade(rs.Shading);
	volMapper->SetRequestedRenderMode(rs.Mode);
}

void iAVolumeRenderer::CreateVolumeMapper(vtkSmartPointer<vtkImageData> imgData)
{
	volMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
	volMapper->SetBlendModeToComposite(); // composite first
	//volMapper->SetRequestedRenderMode(vtkSmartVolumeMapper::RayCastRenderMode);
	volMapper->SetInputData(imgData);
	//volMapper->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->observerFPProgress);
}

double * iAVolumeRenderer::GetOrientation()
{
	return volume->GetOrientation();
}

double * iAVolumeRenderer::GetPosition()
{
	return volume->GetPosition();
}