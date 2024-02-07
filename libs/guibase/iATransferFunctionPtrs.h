// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iATransferFunction.h"

//! An non-owning implementation of iATransferFunction (that is, a container for color- and opacity transfer functions stored elsewhere)
class iAguibase_API iATransferFunctionPtrs : public iATransferFunction
{
public:
	iATransferFunctionPtrs(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf);
	vtkColorTransferFunction* colorTF() override;
	vtkPiecewiseFunction* opacityTF() override;
	void resetFunctions() override;
private:
	vtkColorTransferFunction* m_ctf;
	vtkPiecewiseFunction* m_otf;
};
