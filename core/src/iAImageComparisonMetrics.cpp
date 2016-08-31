/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iAImageComparisonMetrics.h"

#include "iATypedCallHelper.h"
#include "iAToolsITK.h" // for GetITKScalarPixelType


// TODO: check why this function is delivering bogus results for larger images!
template <typename T>
void compareImg_tmpl(iAITKIO::ImagePointer imgB, iAITKIO::ImagePointer refB, iAImageComparisonResult & result)
{
	typedef itk::Image<T, iAITKIO::m_DIM > ImgType;
	ImgType * img = dynamic_cast<ImgType*>(imgB.GetPointer());
	ImgType * ref = dynamic_cast<ImgType*>(refB.GetPointer());
	if (!img || !ref)
	{
		DEBUG_LOG("compareImg_tmpl: One of the images to be compared is NULL!");
		result.equalPixelRate = 0;
		return;
	}
	typename ImgType::RegionType reg = ref->GetLargestPossibleRegion();
	double size = reg.GetSize()[0] * reg.GetSize()[1] * reg.GetSize()[2];
	double sumEqual = 0.0;
#pragma omp parallel for reduction(+:sumEqual)
	for (int i = 0; i < size; ++i)
	{
		if (img->GetBufferPointer()[i] == ref->GetBufferPointer()[i])
		{
			++sumEqual;
		}
	}
	result.equalPixelRate = sumEqual / size;
}

iAImageComparisonResult CompareImages(iAITKIO::ImagePointer img, iAITKIO::ImagePointer reference)
{
	iAImageComparisonResult result;
	ITK_TYPED_CALL(compareImg_tmpl, GetITKScalarPixelType(img), img, reference, result);
	return result;
}
