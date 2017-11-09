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
 
#include "pch.h"
#include "iAModalityTransfer.h"

#include "iAHistogramData.h"
#include "iAImageInfo.h"
//#include "iAToolsVTK.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

iAModalityTransfer::iAModalityTransfer(vtkSmartPointer<vtkImageData> imgData, int binCount)
{
	m_ctf = GetDefaultColorTransferFunction(imgData);
	m_otf = GetDefaultPiecewiseFunction(imgData);
	Update(imgData, binCount);
}

void iAModalityTransfer::Update(vtkSmartPointer<vtkImageData> imgData, int binCount)
{
	if (imgData->GetNumberOfScalarComponents() != 1)
		return;
	m_histogramData = iAHistogramData::Create(imgData, binCount, m_imageInfo);
}


QSharedPointer<iAHistogramData> const iAModalityTransfer::GetHistogramData() const
{
	return m_histogramData;
}

vtkPiecewiseFunction* iAModalityTransfer::GetOpacityFunction()
{
	return m_otf;
}

vtkColorTransferFunction* iAModalityTransfer::GetColorFunction()
{
	return m_ctf;
}

iAImageInfo const & iAModalityTransfer::Info() const
{
	return *(m_imageInfo.data());
}
