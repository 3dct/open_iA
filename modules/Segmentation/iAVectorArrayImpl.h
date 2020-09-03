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

#include "iAVectorArray.h"
#include "iAVectorType.h"
#include "iAVectorTypeImpl.h"

#include <iAImageCoordinate.h>

#include <itkImage.h>

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iAvtkPixelVectorArray: public iAVectorArray
{
public:
	iAvtkPixelVectorArray(int const * dim);
	iAvtkPixelVectorArray(size_t width, size_t height, size_t depth);
	size_t size() const override;
	size_t channelCount() const override;
	QSharedPointer<iAVectorType const> get(size_t voxelIdx) const override;
	iAVectorDataType get(size_t voxelIdx, size_t channelIdx) const override;
	void AddImage(vtkSmartPointer<vtkImageData> img);
private:
	std::vector<vtkSmartPointer<vtkImageData> > m_images;
	iAImageCoordConverter m_coordConv;
};

template <typename ImageType>
class iAitkPixelVectorArray : public iAVectorArray
{
public:
	iAitkPixelVectorArray(size_t width, size_t height, size_t depth);
	size_t size() const override;
	size_t channelCount() const override;
	QSharedPointer<iAVectorType const> get(size_t voxelIdx) const override;
	iAVectorDataType get(size_t voxelIdx, size_t channelIdx) const override;
	void AddImage(itk::SmartPointer<ImageType> img);
private:
	std::vector<itk::SmartPointer<ImageType> > m_images;
	iAImageCoordConverter m_coordConv;
};


template <typename ImageType>
iAitkPixelVectorArray<ImageType>::iAitkPixelVectorArray(size_t width, size_t height, size_t depth):
	m_coordConv(width, height, depth)
{
}

template <typename ImageType>
void iAitkPixelVectorArray<ImageType>::AddImage(itk::SmartPointer<ImageType> img)
{
	typename ImageType::RegionType region = img->GetLargestPossibleRegion();
	typename ImageType::SizeType size = region.GetSize();
	assert(size[0] = m_coordConv.width() &&
		size[1] == m_coordConv.height() &&
		size[2] == m_coordConv.depth());
	m_images.push_back(img);
}

template <typename ImageType>
size_t iAitkPixelVectorArray<ImageType>::size() const
{
	return m_coordConv.vertexCount();
}

template <typename ImageType>
size_t iAitkPixelVectorArray<ImageType>::channelCount() const
{
	return m_images.size();
}

template <typename ImageType>
QSharedPointer<iAVectorType const> iAitkPixelVectorArray<ImageType>::get(size_t voxelIdx) const
{
	return QSharedPointer<iAVectorType const>(new iAPixelVector(*this, voxelIdx));
}

template <typename ImageType>
iAVectorDataType iAitkPixelVectorArray<ImageType>::get(size_t voxelIdx, size_t channelIdx) const
{
	typename ImageType::IndexType idx;
	iAImageCoordinate coords = m_coordConv.coordinatesFromIndex(voxelIdx);
	idx[0] = coords.x;
	idx[1] = coords.y;
	idx[2] = coords.z;
	iAVectorDataType value = m_images[channelIdx]->GetPixel(idx);
	return value;
}
