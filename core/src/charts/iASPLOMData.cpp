/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASPLOMData.h"

#include "iAConsole.h"
#include "iAMathUtility.h"

#include <QTableWidget>

iASPLOMData::iASPLOMData()
{
}

iASPLOMData::iASPLOMData(const QTableWidget * tw)
{
	import(tw);
}

void iASPLOMData::clear()
{
	m_paramNames.clear();
	m_dataPoints.clear();
	m_inverted.clear();
	m_filters.clear();
}

void iASPLOMData::import(const QTableWidget * tw)
{
	clear();
	size_t numParams = tw->columnCount();
	size_t numPoints = tw->rowCount() - 1;
	if (numPoints < 0)
		numPoints = 0;
	for (size_t c = 0; c < numParams; ++c)
	{
		m_paramNames.push_back(tw->item(0, c)->text());
		m_dataPoints.push_back(std::vector<double>());
		std::vector<double> * paramData = &m_dataPoints[c];
		for (size_t r = 1; r < numPoints + 1; ++r)
		{
			paramData->push_back(tw->item(r, c)->text().toDouble());
		}
		m_inverted.push_back(false);
	}
	updateRanges();
}

void iASPLOMData::setParameterNames(std::vector<QString> const & names)
{
	clear();
	m_paramNames = names;
	for (size_t i=0; i<m_paramNames.size(); ++i)
		m_dataPoints.push_back(std::vector<double>());
}

std::vector<std::vector<double>> & iASPLOMData::data()
{
	return m_dataPoints;
}

std::vector<QString> & iASPLOMData::paramNames()
{
	return m_paramNames;
}

const std::vector<std::vector<double>> & iASPLOMData::data() const
{
	return m_dataPoints;
}

const std::vector<double> & iASPLOMData::paramData(size_t paramIndex) const
{
	return m_dataPoints[paramIndex];
}

QString iASPLOMData::parameterName(size_t paramIndex) const
{
	return m_paramNames[paramIndex];
}

size_t iASPLOMData::paramIndex(QString const & paramName) const
{
	for (unsigned long i = 0; i < numParams(); ++i)
		if (m_paramNames[i] == paramName)
			return i;
	return std::numeric_limits<size_t>::max();
}

size_t iASPLOMData::numParams() const
{
	return m_paramNames.size();
}

size_t iASPLOMData::numPoints() const
{
	return m_dataPoints.size() < 1 ? 0 : m_dataPoints[0].size();
}

bool iASPLOMData::isInverted(size_t paramIndex)
{
	return paramIndex < m_inverted.size() ? m_inverted[paramIndex] : false;
}

void iASPLOMData::setInverted(size_t paramIndex, bool isInverted)
{
	m_inverted[paramIndex] = isInverted;
}


bool iASPLOMData::matchesFilter(size_t ind) const
{
	if (m_filters.empty())
		return true;
	for (auto filter: m_filters)
		if (dblApproxEqual(this->paramData(filter.first)[ind], filter.second))
			return true;
	return false;
}

void iASPLOMData::addFilter(size_t paramIndex, double value)
{
	if (paramIndex >= numParams())
	{
		DEBUG_LOG(QString("Invalid filter column ID %1!").arg(paramIndex));
		return;
	}
	m_filters.push_back(std::make_pair(paramIndex, value));
}

void iASPLOMData::removeFilter(size_t paramIndex, double value)
{
	auto searchedPair = std::make_pair(paramIndex, value);
	auto it = std::find(m_filters.begin(), m_filters.end(), searchedPair);
	if (it != m_filters.end())
		m_filters.erase(it);
}

void iASPLOMData::clearFilter()
{
	m_filters.clear();
}

bool iASPLOMData::filterDefined() const
{
	return m_filters.size() > 0;
}

double const* iASPLOMData::paramRange(size_t paramIndex) const
{
	return m_ranges[paramIndex].data();
}

void iASPLOMData::updateRanges()
{
	m_ranges.resize(m_dataPoints.size());
	for (size_t param = 0; param < m_dataPoints.size(); ++param)
		updateRangeInternal(param);
	for (size_t param = 0; param < m_dataPoints.size(); ++param)
		emit dataChanged(param);
}

void iASPLOMData::updateRanges(std::vector<size_t> paramIndices)
{
	for (size_t param: paramIndices)
		updateRangeInternal(param);
	for (size_t param : paramIndices)
		emit dataChanged(param);
}

void iASPLOMData::updateRange(size_t paramIndex)
{
	updateRangeInternal(paramIndex);
	emit dataChanged(paramIndex);
}

void iASPLOMData::updateRangeInternal(size_t paramIndex)
{
	if (paramIndex >= m_dataPoints.size())
		return;
	m_ranges[paramIndex].resize(2);
	m_ranges[paramIndex][0] = std::numeric_limits<double>::max();
	m_ranges[paramIndex][1] = std::numeric_limits<double>::lowest();
	for (size_t row = 0; row < m_dataPoints[paramIndex].size(); ++row)
	{
		double value = m_dataPoints[paramIndex][row];
		if (value < m_ranges[paramIndex][0])
			m_ranges[paramIndex][0] = value;
		if (value > m_ranges[paramIndex][1])
			m_ranges[paramIndex][1] = value;
	}
}
