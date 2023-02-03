// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVectorType.h"	// for iAVectorDataType
#include "iAVectorArray.h"

#include <vector>
#include <cstddef> // for size_t

//! implementation of a standalone vector of values
class iAStandaloneVector: public iAVectorType
{
private:
	std::vector<iAVectorDataType> m_data;
public:
	iAStandaloneVector(IndexType size);
	~iAStandaloneVector() override;
	iAVectorDataType get(size_t channelIdx) const override;
	IndexType size() const override;
	void set(IndexType, iAVectorDataType);
};


//! implementation of a reference to a container type
template<typename ContainerT>
class iARefVector : public iAVectorType
{
private:
	ContainerT const & m_ref;
public:
	iARefVector(ContainerT const & ref);
	iAVectorDataType get(size_t channelIdx) const override;
	IndexType size() const override;
};


//! a single vector accessing its data via the iAVectorArray it is contained in
//! (mainly to directly access the vector for a single pixel from a iAVectorArray
//! drawing its data from a collection of images)
class iAPixelVector: public iAVectorType
{
private:
	iAVectorArray const & m_data;
	size_t m_voxelIdx;
public:
	~iAPixelVector() override;
	iAPixelVector(iAVectorArray const & data, size_t voxelIdx);
	iAVectorDataType get(size_t channelIdx) const override;
	IndexType size() const override;
};




template <typename ContainerT>
iARefVector<ContainerT>::iARefVector(ContainerT const& ref) : m_ref(ref)
{}


template <typename ContainerT>
iAVectorDataType iARefVector<ContainerT>::get(size_t channelIdx) const
{
	assert(static_cast<typename ContainerT::size_type>(channelIdx) < std::numeric_limits<typename ContainerT::size_type>::max());
	return m_ref[static_cast<typename ContainerT::size_type>(channelIdx)];
}

template <typename ContainerT>
iAVectorType::IndexType iARefVector<ContainerT>::size() const
{
	return m_ref.size();
}
