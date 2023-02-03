// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "Segmentation_export.h"

#include <QSharedPointer>

typedef double iAVectorDataType;

//! abstract base class for an arbitrary-sized vector
class Segmentation_API iAVectorType
{
public:
	virtual ~iAVectorType();
	typedef size_t IndexType;
	virtual iAVectorDataType operator[](size_t channelIdx) const;
	virtual iAVectorDataType get(size_t channelIdx) const = 0;
	virtual IndexType size() const = 0;
	virtual QSharedPointer<iAVectorType const> normalized() const;
};
