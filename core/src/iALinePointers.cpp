/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
