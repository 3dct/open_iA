// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASPLOMData.h"

const size_t iASPLOMData::NoDataIdx = std::numeric_limits<size_t>::max();

iASPLOMData::iASPLOMData()
{
}

void iASPLOMData::setParameterNames(std::vector<QString> const & names, size_t rowReserve)
{
	m_paramNames = names;
	m_dataPoints.clear();
	for (size_t i = 0; i < m_paramNames.size(); ++i)
	{
		std::vector<double> column;
		if (rowReserve > 0)
		{
			column.reserve(rowReserve);
		}
		m_dataPoints.push_back(column);
	}
}

void iASPLOMData::addParameter(QString& name)
{
	m_paramNames.push_back(name);
	m_ranges.push_back(std::vector<double>(2, 0));
	m_dataPoints.push_back(std::vector<double>(numPoints()));
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
	{
		if (m_paramNames[i] == paramName)
		{
			return i;
		}
	}
	return NoDataIdx;
}

size_t iASPLOMData::numParams() const
{
	return m_paramNames.size();
}

size_t iASPLOMData::numPoints() const
{
	return m_dataPoints.size() < 1 ? 0 : m_dataPoints[0].size();
}

double const* iASPLOMData::paramRange(size_t paramIndex) const
{
	return m_ranges[paramIndex].data();
}

void iASPLOMData::updateRanges()
{
	m_ranges.resize(m_dataPoints.size());
	for (size_t param = 0; param < m_dataPoints.size(); ++param)
	{
		updateRangeInternal(param);
	}
	for (size_t param = 0; param < m_dataPoints.size(); ++param)
	{
		emit dataChanged(param);
	}
}

void iASPLOMData::updateRanges(std::vector<size_t> paramIndices)
{
	for (size_t param : paramIndices)
	{
		updateRangeInternal(param);
	}
	for (size_t param : paramIndices)
	{
		emit dataChanged(param);
	}
}

void iASPLOMData::updateRange(size_t paramIndex)
{
	updateRangeInternal(paramIndex);
	emit dataChanged(paramIndex);
}

void iASPLOMData::updateRangeInternal(size_t paramIndex)
{
	if (paramIndex >= m_dataPoints.size())
	{
		return;
	}
	m_ranges[paramIndex].resize(2);
	m_ranges[paramIndex][0] = std::numeric_limits<double>::max();
	m_ranges[paramIndex][1] = std::numeric_limits<double>::lowest();
	for (size_t row = 0; row < m_dataPoints[paramIndex].size(); ++row)
	{
		double value = m_dataPoints[paramIndex][row];
		if (value < m_ranges[paramIndex][0])
		{
			m_ranges[paramIndex][0] = value;
		}
		if (value > m_ranges[paramIndex][1])
		{
			m_ranges[paramIndex][1] = value;
		}
	}
}
