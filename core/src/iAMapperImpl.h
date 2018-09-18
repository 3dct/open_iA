/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
#pragma once

#include "iAMapper.h"

#include <cmath>

class iALinearMapper : public iAMapper
{
public:
	iALinearMapper();
	iALinearMapper(double srcMin, double srcMax, double dstMin, double dstMax);
	double srcToDst(double srcVal) const override;
	double dstToSrc(double dstVal) const override;
	bool equals(iAMapper const & other) const override;
	QSharedPointer<iAMapper> clone() override;
	void update(double srcMin, double srcMax, double dstMin, double dstMax) override;
private:
	double m_srcMin, m_dstMin, m_scaleFactor;
};


class iALogarithmicMapper : public iAMapper
{
public:
	iALogarithmicMapper(double srcMin, double srcMax, double dstMin, double dstMax);
	double srcToDst(double srcVal) const override;
	double dstToSrc(double dstVal) const override;
	bool equals(iAMapper const & other) const override;
	QSharedPointer<iAMapper> clone() override;
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

/** Logarithmic convenience function for axes, using base above */
template <typename T>
T LogFunc(T value)
{
	return std::log(value) / std::log(LogBase);
}

