/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iATransferFunction.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

iASimpleTransferFunction::iASimpleTransferFunction(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf) :
	m_ctf(ctf),
	m_otf(otf)
{}

vtkColorTransferFunction * iASimpleTransferFunction::GetColorFunction()
{
	return m_ctf;
}

vtkPiecewiseFunction * iASimpleTransferFunction::GetOpacityFunction()
{
	return m_otf;
}

vtkSmartPointer<vtkColorTransferFunction> GetDefaultColorTransferFunction(vtkSmartPointer<vtkImageData> imageData)
{
	auto cTF = vtkSmartPointer<vtkColorTransferFunction>::New();
	cTF->RemoveAllPoints();
	cTF->AddRGBPoint(imageData->GetScalarRange()[0], 0.0, 0.0, 0.0);
	cTF->AddRGBPoint(imageData->GetScalarRange()[1], 1.0, 1.0, 1.0);
	cTF->Build();
	return cTF;
}

vtkSmartPointer<vtkPiecewiseFunction> GetDefaultPiecewiseFunction(vtkSmartPointer<vtkImageData> imageData)
{
	auto pWF = vtkSmartPointer<vtkPiecewiseFunction>::New();
	pWF->RemoveAllPoints();
	if ( imageData->GetNumberOfScalarComponents() == 1 )
		pWF->AddPoint ( imageData->GetScalarRange()[0], 0.0 );
	else //Set range of rgb, rgba or vector pixel type images to fully opaque
		pWF->AddPoint( imageData->GetScalarRange()[0], 1.0 );
	pWF->AddPoint(imageData->GetScalarRange()[1], 1.0);
	return pWF;
}