// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include "iATransferFunction.h"

#include <vtkSmartPointer.h>

//! Implements iATransferFunction and owns both color and opacity transfer function
class iAguibase_API iATransferFunctionOwner : public iATransferFunction
{
public:
	//! Create an "empty" transfer function
	iATransferFunctionOwner();
	//! Create from the given transfer functions, taking on co-ownership; the transfer functions are
	//!     assumed to be initialized already
	//! @param ctf a smart pointer to an (allocated and initialized) color transfer function
	//! @param otf a smart pointer to an (allocated and initialized) opacity transfer function
	//! @param opacityRamp whether, in case the function is reset through resetFunction, the opacity
	//!     transfer function should go from 0..1 (true) or always be 1 (false)
	iATransferFunctionOwner(vtkSmartPointer<vtkColorTransferFunction> ctf, vtkSmartPointer<vtkPiecewiseFunction> otf, bool opacityRamp=true);
	//! Create own transfer functions and initialize to default for the given range
	//! @param range the range of values for which the transfer function applies
	//! @param opacityRamp whether the created default transfer function (and in case the function
	//!     is reset through resetFunction), the opacity transfer function should go from 0..1 (true)
	//!     or always be 1 (false)
	iATransferFunctionOwner(double const range[2], bool opacityRamp=true);

	//! @{ functions overridden from iATransferFunction:
	vtkPiecewiseFunction* opacityTF() override;
	vtkColorTransferFunction* colorTF() override;
	void resetFunctions() override;
	//! @}

private:
	//! @{ prevent copying; there should always be only one owner of the same set of transfer functions
	iATransferFunctionOwner(iATransferFunctionOwner const& other) = delete;
	iATransferFunctionOwner & operator=(iATransferFunctionOwner const& other) = delete;
	//! @}
	//! internal helper function for resetting to a specified range (could be made public if required)
	void resetFunctions(double const range[2]);


	vtkSmartPointer<vtkColorTransferFunction> m_ctf; //!< the color transfer function
	vtkSmartPointer<vtkPiecewiseFunction> m_otf;     //!< the opacity transfer function
	bool m_opacityRamp;                              //!< whether opacity should go from 0 to 1 (true) or always be 1 in default TF
};
