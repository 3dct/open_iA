// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iATransferFunctionPtrs.h"

#include <iALog.h>

iATransferFunctionPtrs::iATransferFunctionPtrs(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf) :
	m_ctf(ctf),
	m_otf(otf)
{}

vtkColorTransferFunction* iATransferFunctionPtrs::colorTF()
{
	return m_ctf;
}

vtkPiecewiseFunction* iATransferFunctionPtrs::opacityTF()
{
	return m_otf;
}

void iATransferFunctionPtrs::resetFunctions()
{
	LOG(lvlWarn, "iATransferFunctionPtrs::resetFunctions called!");
}
