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
 
#include "pch.h"
#include "iALinePointers.h"

#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkVersion.h>


void iALinePointers::setPointersScaling( double scaling )
{
	double height = iALinePointers::CONE_HEIGHT*scaling;
	for (vtkIdType i=0; i<2; i++)
	{
		pointers[i]->SetHeight(height);
		pointers[i]->SetRadius(height/4.0);
	}
	actors[0]->SetPosition(-height/2, 0, 0);
	actors[1]->SetPosition(height/2, 0, 0);
}

iALinePointers::~iALinePointers()
{
	points->Delete();
	for (vtkIdType i=0; i<2; i++)
	{
		pointers[i]->Delete();
		mappers[i]->Delete();
		actors[i]->Delete();
	}
}

iALinePointers::iALinePointers()
{
	double height = CONE_HEIGHT;
	points = vtkPoints::New();
	points->Allocate(2);
	for (vtkIdType i=0; i<2; i++)
	{
		pointers[i] = vtkConeSource::New();
		pointers[i]->SetCenter(points->GetPoint(i));
		pointers[i]->SetHeight(height);
		pointers[i]->SetRadius(height/4);
		pointers[i]->SetResolution(4);

		mappers[i] = vtkPolyDataMapper::New();
		actors[i] = vtkActor::New();

		mappers[i]->SetInputConnection(pointers[i]->GetOutputPort());
		actors[i]->SetMapper(mappers[i]);
	}
	pointers[1]->SetDirection(-1,0,0);
	actors[0]->SetPosition(-height/2, 0, 0);
	actors[1]->SetPosition(height/2, 0, 0);
}

