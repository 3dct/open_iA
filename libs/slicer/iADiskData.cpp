// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADiskData.h"

#include <vtkActor.h>
#include <vtkDiskSource.h>
#include <vtkPolyDataMapper.h>

iADiskData::iADiskData():
	source(vtkSmartPointer<vtkDiskSource>::New()),
	mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	actor(vtkSmartPointer<vtkActor>::New())
{
	source->SetInnerRadius(0.0);
	source->SetCircumferentialResolution(20);
	mapper->SetInputConnection(source->GetOutputPort());
	actor->SetMapper(mapper);
}
