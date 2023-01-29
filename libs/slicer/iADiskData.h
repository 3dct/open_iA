// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class vtkDiskSource;
class vtkPolyDataMapper;
class vtkActor;

#include <vtkSmartPointer.h>

//! Collection of vtk classes required for displaying a disk.
struct iADiskData
{
	iADiskData();
	vtkSmartPointer<vtkDiskSource> source;
	vtkSmartPointer<vtkPolyDataMapper> mapper;
	vtkSmartPointer<vtkActor> actor;
};
