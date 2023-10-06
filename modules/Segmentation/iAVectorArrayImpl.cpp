// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVectorArrayImpl.h"

iAVectorArray::~iAVectorArray()
{}

iAvtkPixelVectorArray::iAvtkPixelVectorArray(int const * dim):
	m_coordConv(dim[0], dim[1], dim[2])
{
}

iAvtkPixelVectorArray::iAvtkPixelVectorArray(size_t width, size_t height, size_t depth):
	m_coordConv(width, height, depth)
{
}

void iAvtkPixelVectorArray::AddImage(vtkSmartPointer<vtkImageData> img)
{
	int extent[6];
	img->GetExtent(extent);
	assert((extent[1]-extent[0]+1) == m_coordConv.width() &&
		(extent[3]-extent[2]+1) == m_coordConv.height() &&
		(extent[5]-extent[4]+1) == m_coordConv.depth());
	m_images.push_back(img);
}

size_t iAvtkPixelVectorArray::size() const
{
	return m_coordConv.vertexCount();
}

size_t iAvtkPixelVectorArray::channelCount() const
{
	return m_images.size();
}

std::shared_ptr<iAVectorType const> iAvtkPixelVectorArray::get(size_t voxelIdx) const
{
	return std::make_shared<iAPixelVector>(*this, voxelIdx);
}

iAVectorDataType iAvtkPixelVectorArray::get(size_t voxelIdx, size_t channelIdx) const
{
	iAImageCoordinate coords = m_coordConv.coordinatesFromIndex(voxelIdx);
	iAVectorDataType value = m_images[channelIdx]->GetScalarComponentAsDouble(coords.x, coords.y, coords.z, 0);
	return value;
}
