// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

typedef double iADistanceType;

class iANormalizer
{
public:
	virtual ~iANormalizer();
	virtual iADistanceType Normalize(iADistanceType d) const =0;
	virtual void SetMaxValue(iADistanceType maxValue) =0;
	virtual char const * name() const =0;
};
