// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QVector>

#include <cstdint>

struct iASpectrumFilter
{
	iASpectrumFilter(): binIdx(0), minVal(0.0), maxVal(-1.0) {}
	iASpectrumFilter(uint32_t binIdx, double minVal, double maxVal):
		binIdx(binIdx), minVal(minVal), maxVal(maxVal) {}
	uint32_t binIdx;
	double minVal;
	double maxVal;
};

class iASpectrumFilterListener
{
public:
	virtual void OnSelectionUpdate(QVector<iASpectrumFilter> const & filter) =0;
};
