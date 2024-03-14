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
	iADistanceType Normalize(iADistanceType d) const override;
	void SetMaxValue(iADistanceType d) override;
	char const * name() const override;
};

class iALinearNormalizer: public iANormalizer
{
public:
	iALinearNormalizer();
	iADistanceType Normalize(iADistanceType d) const override;
	void SetMaxValue(iADistanceType d) override;
	char const * name() const override;
private:
	iADistanceType m_normalizeFactor;
};

class iAGaussianNormalizer: public iANormalizer
{
public:
	iAGaussianNormalizer();
	iADistanceType Normalize(iADistanceType d) const override;
	void SetMaxValue(iADistanceType d) override;
	char const * name() const override;
	void SetBeta(double beta);
private:
	double m_beta;
	iADistanceType m_maxValue;
	iADistanceType m_valueFactor;

	void UpdateValueFactor();
};

char const * const * GetNormalizerNames();
std::shared_ptr<iANormalizer> CreateNormalizer(QString const & name, double beta);
