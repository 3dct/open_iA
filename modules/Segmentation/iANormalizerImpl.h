// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iANormalizer.h"

#include <QString>

#include <memory>

enum NormalizeModes
{
	tfInvalid = -1,

	nmNone,
	nmLinear,
	nmGaussian,

	// don't change the order or position of these two:
	nmCount,
	nmDefaultNormalizer = nmGaussian
};

class iANoNormalizer: public iANormalizer
{
public:
	virtual iADistanceType Normalize(iADistanceType d) const;
	virtual void SetMaxValue(iADistanceType d);
	virtual char const * name() const;
};

class iALinearNormalizer: public iANormalizer
{
public:
	iALinearNormalizer();
	virtual iADistanceType Normalize(iADistanceType d) const;
	virtual void SetMaxValue(iADistanceType d);
	virtual char const * name() const;
private:
	iADistanceType m_normalizeFactor;
};

class iAGaussianNormalizer: public iANormalizer
{
public:
	iAGaussianNormalizer();
	virtual iADistanceType Normalize(iADistanceType d) const;
	virtual void SetMaxValue(iADistanceType d);
	virtual char const * name() const;
	void SetBeta(double beta);
private:
	double m_beta;
	iADistanceType m_maxValue;
	iADistanceType m_valueFactor;

	void UpdateValueFactor();
};

char const * const * GetNormalizerNames();
std::shared_ptr<iANormalizer> CreateNormalizer(QString const & name, double beta);
