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
#pragma once

#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iAVolumeSettings;
class iATransferFunction;

class vtkActor;
class vtkImageData;
class vtkPlane;
class vtkOpenGLRenderer;
class vtkOutlineFilter;
class vtkPolyDataMapper;
class vtkRenderWindow;
class vtkSmartVolumeMapper;
class vtkVolume;
class vtkVolumeProperty;

class open_iA_Core_API iAVolumeRenderer
{
public:
	iAVolumeRenderer(
		iATransferFunction * transfer,
		vtkSmartPointer<vtkImageData> imgData);
	void ApplySettings(iAVolumeSettings const & rs);
	double * GetOrientation();
	double * GetPosition();
	void SetPosition(double *);
	void SetOrientation(double *);
	void AddToWindow(vtkRenderWindow* w);
	void RemoveFromWindow();
	void iAVolumeRenderer::AddBoundingBoxToWindow(vtkRenderWindow* w);
	void RemoveBoundingBoxFromWindow();
	vtkSmartPointer<vtkVolume> GetVolume();
	void Update();
	void SetCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3);
	void RemoveCuttingPlanes();
	void UpdateBoundingBoxPosition();
private:
	void CreateVolumeMapper(vtkSmartPointer<vtkImageData> imgData);

	vtkSmartPointer<vtkOpenGLRenderer> renderer;
	vtkSmartPointer<vtkVolume> volume;
	vtkSmartPointer<vtkVolumeProperty> volProp;
	vtkSmartPointer<vtkSmartVolumeMapper> volMapper;
	vtkRenderWindow* currentWindow;
	vtkRenderWindow* currentBoundingBoxWindow;

	//! @{ Bounding Box
	vtkSmartPointer<vtkOutlineFilter> outlineFilter;
	vtkSmartPointer<vtkPolyDataMapper> outlineMapper;
	vtkSmartPointer<vtkActor> outlineActor;
	vtkSmartPointer<vtkOpenGLRenderer> outlineRenderer;
	//! @}
};
