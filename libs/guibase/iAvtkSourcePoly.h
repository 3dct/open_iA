// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkSmartPointer.h>

class vtkActor;
class vtkPolyDataMapper;

//! Collection of vtk classes required for displaying some VTK polydata source.
template <class PolySourceClass>
class iAvtkSourcePoly
{
public:
	iAvtkSourcePoly();
	void point(int idx, double * point_out);
	void setPoint(int idx, double x, double y, double z);
	vtkSmartPointer<vtkActor>          actor;
	vtkSmartPointer<vtkPolyDataMapper> mapper;
	vtkSmartPointer<PolySourceClass>   source;
};

template <class PolySourceClass>
iAvtkSourcePoly<PolySourceClass>::iAvtkSourcePoly() :
	actor(vtkSmartPointer<vtkActor>::New()),
	mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	source(vtkSmartPointer<PolySourceClass>::New())
{
	mapper->SetInputConnection(source->GetOutputPort());
	actor->SetMapper(mapper);
}

template <class PolySourceClass>
void iAvtkSourcePoly<PolySourceClass>::setPoint(int idx, double x, double y, double z)
{
	if (idx == 0)
	{
		source->SetPoint1(x, y, z);
	}
	else
	{
		source->SetPoint2(x, y, z);
	}
}

template <class PolySourceClass>
void iAvtkSourcePoly<PolySourceClass>::point(int idx, double* point_out)
{
	if (idx == 0)
	{
		source->GetPoint1(point_out);
	}
	else
	{
		source->GetPoint2(point_out);
	}
}
