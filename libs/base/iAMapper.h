// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

//! Abstract base class for a mapping from a source- into a destination range.
class iAbase_API iAMapper
{
public:
	virtual ~iAMapper();
	virtual double srcToDst(double srcVal) const = 0;
	virtual double dstToSrc(double dstVal) const = 0;
	virtual void update(double srcMin, double srcMax, double dstMin, double dstMax) = 0;
protected:
	virtual bool equals(iAMapper const & other) const;
	friend bool operator==(iAMapper const & a, iAMapper const & b);
};
