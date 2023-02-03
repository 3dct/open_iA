// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVectorType.h"  // for iAVectorDataType

#include <QSharedPointer>

#include <cstddef> // for size_t

//! abstract base class for access to multi-channel/vector data, arranged as array
class iAVectorArray
{
public:
	virtual ~iAVectorArray();
	virtual size_t size() const =0;
	virtual size_t channelCount() const =0;
	virtual QSharedPointer<iAVectorType const> get(size_t voxelIdx) const =0;
	virtual iAVectorDataType get(size_t voxelIdx, size_t channelIdx) const =0;
};
