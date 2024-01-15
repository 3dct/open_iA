// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkParametricSpline.h>

//! Wrapper for vtkParametricSpline to be able to retrieve protected length property.
class iAParametricSpline : public vtkParametricSpline
{
public:
	iAParametricSpline()	{ }
	~iAParametricSpline()	{ }

	static iAParametricSpline *New() {return new iAParametricSpline();}
	double GetLength(){ return this->Length; }
};
