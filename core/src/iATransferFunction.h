/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;

#include <vtkSmartPointer.h>

#include "open_iA_Core_export.h"

//! base class for anything providing a full transfer function (opacity + color)
class open_iA_Core_API iATransferFunction
{
public:
	virtual ~iATransferFunction();
	virtual vtkPiecewiseFunction* opacityTF() =0;
	virtual vtkColorTransferFunction* colorTF() = 0;
};

//! simplest possible transfer function: just a container for ctf and otf
//! (no management of these contained classes!)
class open_iA_Core_API iASimpleTransferFunction : public iATransferFunction
{
public:
	iASimpleTransferFunction(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf);
	vtkColorTransferFunction * colorTF() override;
	vtkPiecewiseFunction * opacityTF() override;
private:
	vtkColorTransferFunction * m_ctf;
	vtkPiecewiseFunction * m_otf;
};

// double range? pass in vtk variables?
open_iA_Core_API vtkSmartPointer<vtkColorTransferFunction> defaultColorTF(double const range[2]);
open_iA_Core_API vtkSmartPointer<vtkPiecewiseFunction> defaultOpacityTF(double const range[2], bool opaqueRamp);

open_iA_Core_API void defaultColorTF(vtkSmartPointer<vtkColorTransferFunction> cTF, double const range[2]);
open_iA_Core_API void defaultOpacityTF(vtkSmartPointer<vtkPiecewiseFunction> pWF, double const range[2], bool opaqueRamp);
