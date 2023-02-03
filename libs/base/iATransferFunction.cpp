// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iATransferFunction.h"

#include "iALog.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QString>    // to be able to create QString from const char *

iATransferFunction::~iATransferFunction()
{}

void defaultColorTF(vtkSmartPointer<vtkColorTransferFunction> cTF, double const range[2])
{
	cTF->RemoveAllPoints();
	cTF->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
	cTF->AddRGBPoint(range[1], 1.0, 1.0, 1.0);
	cTF->Build();
}

void defaultOpacityTF(vtkSmartPointer<vtkPiecewiseFunction> pWF, double const range[2], bool opacityRamp)
{
	pWF->RemoveAllPoints();
	pWF->AddPoint(range[0], opacityRamp ? 0.0 : 1.0);
	pWF->AddPoint(range[1], 1.0);
}

vtkSmartPointer<vtkColorTransferFunction> defaultColorTF(double const range[2])
{
	auto cTF = vtkSmartPointer<vtkColorTransferFunction>::New();
	defaultColorTF(cTF, range);
	return cTF;
}

vtkSmartPointer<vtkPiecewiseFunction> defaultOpacityTF(double const range[2], bool opacityRamp)
{
	auto pWF = vtkSmartPointer<vtkPiecewiseFunction>::New();
	defaultOpacityTF(pWF, range, opacityRamp);
	return pWF;
}
