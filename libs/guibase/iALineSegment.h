// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <vtkSmartPointer.h>

class vtkActor;
class vtkPolyDataMapper;
class vtkLineSource;

//! Collection of vtk classes required for displaying a line segment.
struct iAguibase_API iALineSegment
{
	iALineSegment();
	void point(int idx, double * point_out);
	void setPoint(int idx, double x, double y, double z);
	vtkSmartPointer<vtkActor>			actor;
	vtkSmartPointer<vtkPolyDataMapper>	mapper;
	vtkSmartPointer<vtkLineSource>		lineSource;
};
