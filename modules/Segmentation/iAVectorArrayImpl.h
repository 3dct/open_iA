// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVectorArray.h"
#include "iAVectorType.h"
#include "iAVectorTypeImpl.h"

#include <iAImageCoordinate.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#include <itkImage.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <memory>

class iAvtkPixelVectorArray: public iAVectorArray
{
public:
	iAvtkPixelVectorArray(int const * dim);
	size_t size() const override;
	size_t channelCount() const override;
	std::shared_ptr<iAVectorType const> get(size_t voxelIdx) const override;
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
	std::shared_ptr<iAVectorType const> get(size_t voxelIdx) const override;
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
std::shared_ptr<iAVectorType const> iAitkPixelVectorArray<ImageType>::get(size_t voxelIdx) const
{
	return std::make_shared<iAPixelVector>(*this, voxelIdx);
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
