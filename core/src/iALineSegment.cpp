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
#include "iALineSegment.h"

#include <vtkActor.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkVersion.h>

iALineSegment::~iALineSegment()
{
	points->Delete();
	lineSource->Delete();
	mapper->Delete();
	actor->Delete();
}

iALineSegment::iALineSegment()
{
	points = vtkPoints::New();
	points->Allocate(2);
	points->SetPoint(0, 0,0,0);
	points->SetPoint(1, 1,1,1);
	lineSource = vtkLineSource::New();
	mapper = vtkPolyDataMapper::New();
	actor = vtkActor::New();

	lineSource->SetPoint1(points->GetPoint(0));
	lineSource->SetPoint2(points->GetPoint(1));
	lineSource->SetResolution(1);
	mapper->SetInputConnection(lineSource->GetOutputPort());
	actor->SetMapper(mapper);
}

