/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
 
#include "dlg_planeSlicer.h"

#include <QVTKWidget2.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageProperty.h>
#include <vtkImageSlice.h>
#include <vtkOpenGLRenderer.h>

dlg_planeSlicer::dlg_planeSlicer() :
	m_vtkWidget(new QVTKWidget2(this))
{
	m_renderer = vtkSmartPointer<vtkOpenGLRenderer>::New();
	m_vtkWidget->GetRenderWindow()->AddRenderer(m_renderer);
	slicer->layout()->addWidget(m_vtkWidget);
	m_renderer->SetBackground(1, 1, 1);

	m_camera_ext = m_renderer->GetActiveCamera();
	m_camera_ext->ParallelProjectionOn();
	m_camera_ext->SetParallelScale(100); // TODO: use "image size" here
}

int dlg_planeSlicer::AddImage(vtkSmartPointer<vtkImageData> image, vtkSmartPointer<vtkColorTransferFunction> lut, double initialOpacity)
{
	vtkSmartPointer<vtkImageResliceMapper> mapper = vtkSmartPointer<vtkImageResliceMapper>::New();
	mapper->SliceFacesCameraOn();
	mapper->SliceAtFocalPointOn();
	vtkSmartPointer<vtkImageSlice> imageSlice = vtkSmartPointer<vtkImageSlice>::New();
	vtkSmartPointer<vtkImageMapToColors> map = vtkSmartPointer<vtkImageMapToColors>::New();
	map->SetInputData(image);
	map->SetLookupTable(lut);
	imageSlice->SetMapper(mapper);
	vtkImageProperty* prop = imageSlice->GetProperty();
	prop->SetOpacity(initialOpacity);
	imageSlice->SetProperty(prop);
	mapper->SetInputConnection(map->GetOutputPort());

	m_images.push_back(imageSlice);
	
	m_renderer->AddViewProp(imageSlice);

	return m_images.size() - 1;
}

void dlg_planeSlicer::SetOpacity(int imageIdx, double opacity)
{
	vtkImageProperty* prop = m_images[imageIdx]->GetProperty();
	prop->SetOpacity(opacity);	 // TODO: make adaptable!
	m_images[imageIdx]->SetProperty(prop);
}

void dlg_planeSlicer::SetCuttingPlane(double pos[3], double n[3])
{
	const double d = 1;
	//m_mapper->SetSlicePlane(plane);
	m_camera_ext->SetFocalPoint(pos);
	m_camera_ext->SetPosition(pos[0] + d*n[0], pos[1] + d*n[1], pos[2] + d*n[2]);
	m_renderer->ResetCameraClippingRange();
}
