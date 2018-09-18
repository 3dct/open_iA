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
#include "iAMapperImpl.h"

#include "iAConsole.h"
#include "iAMathUtility.h"

// iAMapper

iAMapper::~iAMapper() {}

bool iAMapper::equals(iAMapper const & other) const
{
	return false;
}

bool operator==(iAMapper const & a, iAMapper const & b)
{
	return typeid(a) == typeid(b) // Allow compare only instances of the same dynamic type
		&& a.equals(b);           // If types are the same then do the comparision.
}


// iALinearMapper

iALinearMapper::iALinearMapper():
	m_srcMin(0), m_dstMin(0), m_scaleFactor(1)
{}

iALinearMapper::iALinearMapper(double srcMin, double srcMax, double dstMin, double dstMax)
{
	iALinearMapper::update(srcMin, srcMax, dstMin, dstMax);
}

double iALinearMapper::srcToDst(double srcVal) const
{
	return ((srcVal - m_srcMin) * m_scaleFactor) + m_dstMin;
}

double iALinearMapper::dstToSrc(double dstVal) const
{
	return ((dstVal - m_dstMin) / m_scaleFactor) + m_srcMin;
}

bool iALinearMapper::equals(iAMapper const & other) const
{
	// same type check already done in operator== above
	iALinearMapper const & linearOther = dynamic_cast<iALinearMapper const &>(other);
	return (m_scaleFactor == linearOther.m_scaleFactor &&
		m_srcMin == linearOther.m_srcMin && m_dstMin == linearOther.m_dstMin);
}

void iALinearMapper::update(double srcMin, double srcMax, double dstMin, double dstMax)
{
	m_srcMin = srcMin;
	m_dstMin = dstMin;
	m_scaleFactor = (dstMax - m_dstMin) / (srcMax - m_srcMin);
}


// iALogarithmicMapper

iALogarithmicMapper::iALogarithmicMapper(double srcMin, double srcMax, double dstMin, double dstMax)
{
	iALogarithmicMapper::update(srcMin, srcMax, dstMin, dstMax);
}

double iALogarithmicMapper::srcToDst(double srcVal) const
{
	if (srcVal <= 0)
	{
		DEBUG_LOG(QString("Value %1 cannot be logarithmically mapped as it is <= 0!").arg(srcVal));
		return 0;
	}
	double srcLog = clamp(m_srcMinLog, m_srcMaxLog, LogFunc(srcVal));
	return m_internalMapper.srcToDst(srcLog);
}

double iALogarithmicMapper::dstToSrc(double dstVal) const
{
	double srcLog = m_internalMapper.dstToSrc(dstVal);
	return std::pow(LogBase, srcLog);
}
bool iALogarithmicMapper::equals(iAMapper const & other) const
{
	// same type check already done in operator== above
	iALogarithmicMapper const & logOther = dynamic_cast<iALogarithmicMapper const &>(other);
	return (m_srcMinLog == logOther.m_srcMinLog && m_srcMaxLog == logOther.m_srcMaxLog &&
		m_internalMapper.equals(logOther.m_internalMapper));
}

void iALogarithmicMapper::update(double srcMin, double srcMax, double dstMin, double dstMax)
{
	if (srcMin <= 0 || srcMax <= 0)
	{
		DEBUG_LOG(QString("Invalid logarithmic mapping, can only map values > 0 (was given range [%1, %2])").arg(srcMin).arg(srcMax));
		return;
	}
	m_srcMinLog = LogFunc(srcMin);
	m_srcMaxLog = LogFunc(srcMax);
	m_internalMapper.update(m_srcMinLog, m_srcMaxLog, dstMin, dstMax);
}
