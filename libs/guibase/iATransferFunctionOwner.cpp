// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iATransferFunctionOwner.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <iALog.h>

#include <cassert>

iATransferFunctionOwner::iATransferFunctionOwner():
	m_ctf(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_otf(vtkSmartPointer<vtkPiecewiseFunction>::New())
{}

iATransferFunctionOwner::iATransferFunctionOwner(vtkSmartPointer<vtkColorTransferFunction> ctf, vtkSmartPointer<vtkPiecewiseFunction> otf, bool opacityRamp):
	m_ctf(ctf),
	m_otf(otf),
	m_opacityRamp(opacityRamp)
{}

iATransferFunctionOwner::iATransferFunctionOwner(double const range[2], bool opacityRamp):
	m_ctf(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_otf(vtkSmartPointer<vtkPiecewiseFunction>::New()),
	m_opacityRamp(opacityRamp)
{
	resetFunctions(range);
}

void iATransferFunctionOwner::resetFunctions()
{
	double range[2];         // we need to use the GetRange version with parameters here; the other returns a
	m_ctf->GetRange(range);  // reference to the internals, and would lead to it being modified while being used!
	resetFunctions(range);
}

vtkPiecewiseFunction* iATransferFunctionOwner::opacityTF()
{
	assert(m_otf);
	return m_otf;
}

vtkColorTransferFunction* iATransferFunctionOwner::colorTF()
{
	assert(m_ctf);
	return m_ctf;
}

void iATransferFunctionOwner::resetFunctions(double const range[2])
{
	defaultColorTF(m_ctf, range);
	defaultOpacityTF(m_otf, range, m_opacityRamp);
}
