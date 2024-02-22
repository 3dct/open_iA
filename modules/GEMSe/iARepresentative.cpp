// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARepresentative.h"

#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <QVector>

template <class T>
void diff_marker_tmpl(QVector<iAITKIO::ImagePointer> imgsBase, double differenceMarkerValue, iAITKIO::ImagePointer & result)
{
	typedef itk::Image<T, iAITKIO::Dim > ImgType;
	QVector<ImgType*> imgs;
	for (int i = 0; i < imgsBase.size(); ++i)
	{
		auto ptr = dynamic_cast<ImgType*>(imgsBase[i].GetPointer());
		if (!ptr)
		{
			LOG(lvlError, "Difference Marker: Invalid type conversion - images must have same type!");
			return;
		}
		imgs.push_back(ptr);
	}
	typename ImgType::Pointer out = createImage<ImgType>(imgs[0]);
	typename iAITKIO::ImageBaseType::RegionType reg = imgs[0]->GetLargestPossibleRegion();
	typename iAITKIO::ImageBaseType::SizeType size = reg.GetSize();
	typename iAITKIO::ImageBaseType::IndexType idx;
	for (idx[0] = 0; idx[0] >= 0 && static_cast<uint64_t>(idx[0]) < size[0]; ++idx[0])
	{	// >= 0 checks to prevent signed int overflow!
		for (idx[1] = 0; idx[1] >= 0 && static_cast<uint64_t>(idx[1]) < size[1]; ++idx[1])
		{
			for (idx[2] = 0; idx[2] >= 0 && static_cast<uint64_t>(idx[2]) < size[2]; ++idx[2])
			{
				double pixel = imgs[0]->GetPixel(idx);
				for (int i = 1; i < imgs.size(); ++i)
				{
					if (imgs[i]->GetPixel(idx) != pixel)
					{
						pixel = differenceMarkerValue;
					}
				}
				out->SetPixel(idx, pixel);
			}
		}
	}
	result = out;
}

iAITKIO::ImagePointer CalculateDifferenceMarkers(QVector<iAITKIO::ImagePointer> imgs, double differenceMarkerValue)
{
	if (imgs.size() == 0) // all child images filtered out
	{
		return iAITKIO::ImagePointer();
	}
	else if (imgs.size() == 1)	// one of the childs filtered out
	{
		return imgs[0];
	}
	iAITKIO::ImagePointer result;
	ITK_TYPED_CALL(diff_marker_tmpl, itkScalarType(imgs[0]), imgs, differenceMarkerValue, result);
	return result;
}
