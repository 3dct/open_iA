// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALineSegment.h"

#include <vtkActor.h>
#include <vtkLineSource.h>
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
