// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAMapper.h"

#include "iAbase_export.h"

#include <cmath>

//! Maps linearly from source to destination range.
class iAbase_API iALinearMapper : public iAMapper
{
public:
	iALinearMapper();
	iALinearMapper(double srcMin, double srcMax, double dstMin, double dstMax);
	double srcToDst(double srcVal) const override;
	double dstToSrc(double dstVal) const override;
	bool equals(iAMapper const & other) const override;
	void update(double srcMin, double srcMax, double dstMin, double dstMax) override;
private:
	double m_srcMin, m_dstMin, m_scaleFactor;
};

//! Maps logarithmically from source to destination range.
class iAbase_API iALogarithmicMapper : public iAMapper
{
public:
	iALogarithmicMapper(double srcMin, double srcMax, double dstMin, double dstMax);
	double srcToDst(double srcVal) const override;
	double dstToSrc(double dstVal) const override;
	bool equals(iAMapper const & other) const override;
	void update(double srcMin, double srcMax, double dstMin, double dstMax) override;
private:
	double m_srcMinLog, m_srcMaxLog;
	iALinearMapper m_internalMapper;
};

namespace
{
	//! Logarithmic base used for diagram axes
	const double LogBase = 2.0;
}

//! Logarithmic convenience function for axes, using base above.
template <typename T>
T LogFunc(T value)
{
	return std::log(value) / std::log(LogBase);
}
