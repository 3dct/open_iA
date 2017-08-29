/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAImageWidget.h"

#include "iATransferFunction.h"

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkRenderer.h>


iAImageWidget::iAImageWidget(vtkSmartPointer<vtkImageData> img)
{
	auto ctf = GetDefaultColorTransferFunction(img);
	/*
	auto mapIntensityToColor = vtkSmartPointer<vtkImageMapToColors>::New();
	mapIntensityToColor->SetInputData(img);
	mapIntensityToColor->SetLookupTable(ctf);
	*/

	auto mapper = vtkSmartPointer<vtkImageResliceMapper>::New();
	mapper->SetInputData(img);
	//resliceMapper->SetInputConnection(mapIntensityToColor->GetOutputPort());
	/*
	vtkNew<vtkPlane> plane;
	plane->SetOrigin(0, 0, 0);
	plane->SetNormal(0, 0, 1);
	resliceMapper->SetSlicePlane(plane);
	*/
	// resliceMapper->SetSlabThickness(0.1);
	mapper->SliceAtFocalPointOn();
	mapper->BorderOn();

	auto slice = vtkSmartPointer<vtkImageSlice>::New();
	slice->SetMapper(mapper);

	auto imgProp = vtkSmartPointer<vtkImageProperty>::New();
	imgProp->SetLookupTable(ctf);
	slice->SetProperty(imgProp);

	auto renderer = vtkSmartPointer<vtkRenderer>::New();
	renderer->AddViewProp(slice);
	renderer->ResetCamera();
	auto renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	// renderWindow->SetSize(300, 300);
	renderWindow->AddRenderer(renderer);

	SetRenderWindow(renderWindow);

	/*
	// Setup render window interactor
	vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
		vtkSmartPointer<vtkRenderWindowInteractor>::New();

	vtkSmartPointer<vtkInteractorStyleImage> style =
		vtkSmartPointer<vtkInteractorStyleImage>::New();
	renderWindowInteractor->SetInteractorStyle(style);

	// Render and start interaction
	renderWindowInteractor->SetRenderWindow(renderWindow);
	renderWindowInteractor->Initialize();

	renderWindowInteractor->Start();
	*/
}
