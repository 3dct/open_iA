// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALinePointers.h"

#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

iALinePointers::iALinePointers():
	points(vtkSmartPointer<vtkPoints>::New())
{
	points->Allocate(2);
	for (vtkIdType i = 0; i < 2; i++)
	{
		pointers[i] = vtkSmartPointer<vtkConeSource>::New();
		mappers[i] = vtkSmartPointer<vtkPolyDataMapper>::New();
		actors[i] = vtkSmartPointer<vtkActor>::New();
		pointers[i]->SetResolution(4);
		mappers[i]->SetInputConnection(pointers[i]->GetOutputPort());
		actors[i]->SetMapper(mappers[i]);
		actors[i]->GetProperty()->SetAmbientColor(1.0, 1.0, 1.0);
		actors[i]->GetProperty()->SetAmbient(1.0);
		//actors[i]->GetProperty()->SetLineWidth(1);
	}
	pointers[1]->SetDirection(-1, 0, 0);
}

void iALinePointers::setVisible(bool visible)
{
	for (vtkIdType i = 0; i < 2; i++)
		actors[i]->SetVisibility(visible);
}

void iALinePointers::addToRenderer(vtkRenderer * ren)
{
	for (vtkIdType i = 0; i < 2; i++)
		ren->AddActor(actors[i]);
}

void iALinePointers::updatePosition(double posY, double zeroLevelPosY, double startX, double endX, double const * spacing)
{
	points->SetPoint(0, startX, zeroLevelPosY, ZCoord);
	points->SetPoint(1, endX, zeroLevelPosY, ZCoord);
	double scaling = spacing[0] > spacing[1] ? spacing[0] : spacing[1];
	double height = ConeHeight * scaling;
	for (int i = 0; i < 2; ++i)
	{
		pointers[i]->SetHeight(height);
		pointers[i]->SetRadius(height / 4.0);
		pointers[i]->SetCenter(points->GetPoint(i));
	}
	actors[0]->SetPosition(-height / 2, posY, 0);
	actors[1]->SetPosition( height / 2, posY, 0);
}
