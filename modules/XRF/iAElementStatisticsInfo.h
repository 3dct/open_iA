/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

class iAElementStatisticsInfo
{
public:
	iAElementStatisticsInfo():
		min(std::numeric_limits<double>::max()),
		max(-std::numeric_limits<double>::max()),
		median(0.0),
		mean(0.0),
		variance(0.0),
		kurtosis(0.0),
		skewness(0.0)
	{}
	QString elementName;
	double min, max, median, mean, variance, kurtosis, skewness;
};

class iALabelStatisticsInfo
{
public:
	typedef QVector<iAElementStatisticsInfo> ContainerType;
	typedef ContainerType::const_iterator const_iterator;
	
	iALabelStatisticsInfo():
		count(0)
	{}
	iALabelStatisticsInfo(int size):
		m_elementStats(size),
		count(0)
	{}
	const_iterator begin() const
	{
		return m_elementStats.begin();
	}
	const_iterator end() const
	{
		return m_elementStats.end();
	}
	iAElementStatisticsInfo & operator[](int idx)
	{
		return m_elementStats[idx];
	}
	ContainerType m_elementStats;
	int count;
};
