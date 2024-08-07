// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "segmentation_export.h"

#include <memory>

typedef double iAVectorDataType;

//! abstract base class for an arbitrary-sized vector
class Segmentation_API iAVectorType
{
public:
	virtual ~iAVectorType();
	typedef size_t IndexType;
	virtual iAVectorDataType operator[](IndexType channelIdx) const;
	virtual iAVectorDataType get(IndexType channelIdx) const = 0;
	virtual IndexType size() const = 0;
	virtual std::shared_ptr<iAVectorType const> normalized() const;
};
