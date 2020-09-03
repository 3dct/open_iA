/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iALineSegment.h"

#include <vtkActor.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>

iALineSegment::iALineSegment():
	actor(vtkSmartPointer<vtkActor>::New()),
	mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	lineSource(vtkSmartPointer<vtkLineSource>::New())
{
	lineSource->SetPoint1(0, 0, 0);
	lineSource->SetPoint2(1, 1, 1);
	lineSource->SetResolution(1);
	mapper->SetInputConnection(lineSource->GetOutputPort());
	actor->SetMapper(mapper);
}

void iALineSegment::setPoint(int idx, double x, double y, double z)
{
	if (idx == 0)
	{
		lineSource->SetPoint1(x, y, z);
	}
	else
	{
		lineSource->SetPoint2(x, y, z);
	}
}

void iALineSegment::point(int idx, double* point_out)
{
	if (idx == 0)
	{
		lineSource->GetPoint1(point_out);
	}
	else
	{
		lineSource->GetPoint2(point_out);
	}
}
