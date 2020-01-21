/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#pragma once

#include "iANormalizer.h"

#include <QSharedPointer>
#include <QString>

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
	virtual char const * const name() const;
};

class iALinearNormalizer: public iANormalizer
{
public:
	iALinearNormalizer();
	virtual iADistanceType Normalize(iADistanceType d) const;
	virtual void SetMaxValue(iADistanceType d);
	virtual char const * const name() const;
private:
	iADistanceType m_normalizeFactor;
};

class iAGaussianNormalizer: public iANormalizer
{
public:
	iAGaussianNormalizer();
	virtual iADistanceType Normalize(iADistanceType d) const;
	virtual void SetMaxValue(iADistanceType d);
	virtual char const * const name() const;
	void SetBeta(double beta);
private:
	double m_beta;
	iADistanceType m_maxValue;
	iADistanceType m_valueFactor;

	void UpdateValueFactor();
};

char const * const * const GetNormalizerNames();
QSharedPointer<iANormalizer> CreateNormalizer(QString const & name, double beta);
