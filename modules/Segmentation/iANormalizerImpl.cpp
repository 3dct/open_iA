// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iANormalizerImpl.h"

#include <cassert>
#include <cmath>

char const * const TransformNames[3] = {
	"None",
	"Divide by max",
	"Gaussian"
};

iANormalizer::~iANormalizer()
{}

iADistanceType iANoNormalizer::Normalize(iADistanceType d) const
{
	return d;
}

void iANoNormalizer::SetMaxValue(iADistanceType )
{
}

char const * iANoNormalizer::name() const
{
	return TransformNames[nmNone];
}




iALinearNormalizer::iALinearNormalizer():
	m_normalizeFactor(1.0)
{}

void iALinearNormalizer::SetMaxValue(iADistanceType maxValue)
{
	m_normalizeFactor = 1.0 / maxValue;
}

iADistanceType iALinearNormalizer::Normalize(iADistanceType d) const
{
	// TODO: make sure data is normalize first?
	assert(d >= 0 && d <= 1);
	return d * m_normalizeFactor;
}

char const * iALinearNormalizer::name() const
{
	return TransformNames[nmLinear];
}

iAGaussianNormalizer::iAGaussianNormalizer():
	m_beta(0.5),
	m_maxValue(1.0)
{
}

void iAGaussianNormalizer::UpdateValueFactor()
{
	m_valueFactor = -m_beta/m_maxValue;
}

iADistanceType iAGaussianNormalizer::Normalize(iADistanceType d) const
{
	return 1 - std::exp(m_valueFactor * d );
}

void iAGaussianNormalizer::SetBeta(double beta)
{
	m_beta = beta;
	UpdateValueFactor();
}

void iAGaussianNormalizer::SetMaxValue(iADistanceType maxValue)
{
	m_maxValue = maxValue;
	UpdateValueFactor();
}

char const * iAGaussianNormalizer::name() const
{
	return TransformNames[nmGaussian];
}

std::shared_ptr<iANormalizer> CreateNormalizer(QString const & name, double beta)
{
	if (name == TransformNames[nmLinear])
		return std::make_shared<iALinearNormalizer>();
	else if (name == TransformNames[nmGaussian])
	{
		iAGaussianNormalizer* norm = new iAGaussianNormalizer;
		norm->SetBeta(beta);
		return std::shared_ptr<iANormalizer>(norm);
	}
	else
		return std::make_shared<iANoNormalizer>();
}


char const * const * GetNormalizerNames()
{
	return TransformNames;
}
