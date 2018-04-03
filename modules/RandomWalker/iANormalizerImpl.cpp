/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "iANormalizerImpl.h"

#include <cassert>
#include <cmath>

char const * const TransformNames[3] = {
	"None",
	"Divide by max",
	"Gaussian"
};

iADistanceType iANoNormalizer::Normalize(iADistanceType d) const
{
	return d;
}

void iANoNormalizer::SetMaxValue(iADistanceType )
{
}

char const * const iANoNormalizer::GetName() const
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

char const * const iALinearNormalizer::GetName() const
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
	return 1 - exp(m_valueFactor * d );
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

char const * const iAGaussianNormalizer::GetName() const
{
	return TransformNames[nmGaussian];
}

QSharedPointer<iANormalizer> CreateNormalizer(QString const & name, double beta)
{
	if (name == TransformNames[nmLinear])
		return QSharedPointer<iANormalizer>(new iALinearNormalizer);
	else if (name == TransformNames[nmGaussian])
	{
		iAGaussianNormalizer* norm = new iAGaussianNormalizer;
		norm->SetBeta(beta);
		return QSharedPointer<iANormalizer>(norm);
	}
	else
		return QSharedPointer<iANormalizer>(new iANoNormalizer);
}


char const * const * const GetNormalizerNames()
{
	return TransformNames;
}