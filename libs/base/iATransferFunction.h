// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

#include <vtkSmartPointer.h>

#include "iabase_export.h"

//! Base class for anything providing a full transfer function (opacity + color)
class iAbase_API iATransferFunction
{
public:
	virtual ~iATransferFunction();
	// TODO: replace with smart pointers?
	virtual vtkPiecewiseFunction* opacityTF() =0;
	virtual vtkColorTransferFunction* colorTF() = 0;
	virtual void resetFunctions() = 0;
	//! make sure the contained transfer function matches the given range, adapt TF if necessary
	//! in order to better encapsulate modification, this would probably better not be allowed here
	void ensureValidity(double range[2]);
};

// double range? pass in vtk variables?
iAbase_API vtkSmartPointer<vtkColorTransferFunction> defaultColorTF(double const range[2]);
iAbase_API vtkSmartPointer<vtkPiecewiseFunction> defaultOpacityTF(double const range[2], bool opacityRamp);

iAbase_API void defaultColorTF(vtkSmartPointer<vtkColorTransferFunction> cTF, double const range[2]);
iAbase_API void defaultOpacityTF(vtkSmartPointer<vtkPiecewiseFunction> pWF, double const range[2], bool opacityRamp);
