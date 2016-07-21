/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;

#include <vtkSmartPointer.h>

#include "open_iA_Core_export.h"

//! base class for anything providing a full transfer function (opacity + color)
class iATransferFunction
{
public:
	virtual vtkPiecewiseFunction* GetOpacityFunction() =0;
	virtual vtkColorTransferFunction* GetColorFunction() = 0;
};

//! simplest possible transfer function: just a container for ctf and otf
//! (no management of these contained classes!)
class open_iA_Core_API iASimpleTransferFunction : public iATransferFunction
{
public:
	iASimpleTransferFunction(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf);
	virtual vtkColorTransferFunction * GetColorFunction();
	virtual vtkPiecewiseFunction * GetOpacityFunction();
private:
	vtkColorTransferFunction * m_ctf;
	vtkPiecewiseFunction * m_otf;
};

// double range? pass in vtk variables?
open_iA_Core_API vtkSmartPointer<vtkColorTransferFunction> GetDefaultColorTransferFunction(vtkSmartPointer<vtkImageData> imageData);
open_iA_Core_API vtkSmartPointer<vtkPiecewiseFunction> GetDefaultPiecewiseFunction(vtkSmartPointer<vtkImageData> imageData);
