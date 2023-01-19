/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAguibase_export.h"

#include "iATransferFunction.h"

#include <vtkSmartPointer.h>

//! Implements iATransferFunction and owns both color and opacity transfer function
class iAguibase_API iATransferFunctionOwner : public iATransferFunction
{
public:
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
