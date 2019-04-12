/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iARepresentative.h"

#include <iAToolsITK.h>

#include <QVector>

template <class T>
void diff_marker_tmpl(QVector<iAITKIO::ImagePointer> imgsBase, int differenceMarkerValue, iAITKIO::ImagePointer & result)
{
	typedef itk::Image<T, iAITKIO::m_DIM > ImgType;
	QVector<ImgType*> imgs;
	for (int i = 0; i < imgsBase.size(); ++i)
	{
		imgs.push_back(dynamic_cast<ImgType*>(imgsBase[i].GetPointer()));
	}
	typename ImgType::Pointer out = createImage<ImgType>(imgs[0]);
	typename iAITKIO::ImageBaseType::RegionType reg = imgs[0]->GetLargestPossibleRegion();
	typename iAITKIO::ImageBaseType::SizeType size = reg.GetSize();
	typename iAITKIO::ImageBaseType::IndexType idx;
	for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
	{
		for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
		{
			for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
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

iAITKIO::ImagePointer CalculateDifferenceMarkers(QVector<iAITKIO::ImagePointer> imgs, int differenceMarkerValue)
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
	ITK_TYPED_CALL(diff_marker_tmpl, itkScalarPixelType(imgs[0]), imgs, differenceMarkerValue, result);
	return result;
}
