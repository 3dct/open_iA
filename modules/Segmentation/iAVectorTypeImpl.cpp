// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVectorTypeImpl.h"

iAVectorType::~iAVectorType()
{}

iAVectorDataType iAVectorType::operator[](size_t channelIdx) const
{
	return get(channelIdx);
}

std::shared_ptr<iAVectorType const> iAVectorType::normalized() const
{
	std::shared_ptr<iAStandaloneVector> result(new iAStandaloneVector(size()));
	iAVectorDataType sum = 0;
	for(iAVectorType::IndexType i = 0; i<size(); ++i)
	{
		sum += get(i);
	}
	for(iAVectorType::IndexType i = 0; i<size(); ++i)
	{
		result->set(i, get(i) / sum);
	}
	return result;
}



iAStandaloneVector::~iAStandaloneVector()
{}

iAStandaloneVector::iAStandaloneVector(IndexType size):
	m_data(size)
{}

iAVectorDataType iAStandaloneVector::get(size_t idx) const
{
	return m_data[idx];
}

iAVectorType::IndexType iAStandaloneVector::size() const
{
	return m_data.size();
}

void iAStandaloneVector::set(iAVectorType::IndexType idx, iAVectorDataType value)
{
	m_data[idx] = value;
}



iAPixelVector::~iAPixelVector()
{}

iAPixelVector::iAPixelVector(iAVectorArray const& data, size_t voxelIdx) :
	m_data(data),
	m_voxelIdx(voxelIdx)
{}

iAVectorDataType iAPixelVector::get(size_t channelIdx) const
{
	iAVectorDataType value = m_data.get(m_voxelIdx, channelIdx);
	return value;
}

iAVectorType::IndexType iAPixelVector::size() const
{
	return m_data.channelCount();
}
