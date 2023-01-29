// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageComparisonMetrics.h"

#include "iATypedCallHelper.h"
#include "iAToolsITK.h" // for itkScalarType


// TODO: check why this function is delivering bogus results for larger images!
template <typename T>
void compareImg_tmpl(iAITKIO::ImagePointer imgB, iAITKIO::ImagePointer refB, iAImageComparisonResult & result)
{
	typedef itk::Image<T, iAITKIO::Dim > ImgType;
	ImgType * img = dynamic_cast<ImgType*>(imgB.GetPointer());
	ImgType * ref = dynamic_cast<ImgType*>(refB.GetPointer());
	if (!img || !ref)
	{
		LOG(lvlError, "compareImg_tmpl: One of the images to be compared is nullptr!");
		result.equalPixelRate = 0;
		return;
	}
	typename ImgType::RegionType reg = ref->GetLargestPossibleRegion();
	long long size = reg.GetSize()[0] * reg.GetSize()[1] * reg.GetSize()[2];
	double sumEqual = 0.0;
#pragma omp parallel for reduction(+:sumEqual)
	for (long long i = 0; i < size; ++i)
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
	ITK_TYPED_CALL(compareImg_tmpl, itkScalarType(img), img, reference, result);
	return result;
}
