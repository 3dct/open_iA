// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAXYPlotData.h"

#include <cassert>

iAPlotData::DataType iAXYPlotData::xValue(size_t idx) const
{
	assert(idx < m_values.size());
	return m_values[idx].first;
}

iAPlotData::DataType iAXYPlotData::yValue(size_t idx) const
{
	assert(idx < m_values.size());
	return m_values[idx].second;
}

iAPlotData::DataType const* iAXYPlotData::xBounds() const
{
	return m_xBounds;
}

iAPlotData::DataType const* iAXYPlotData::yBounds() const
{
	return m_yBounds;
}

size_t iAXYPlotData::valueCount() const
{
	return m_values.size();
}

void iAXYPlotData::addValue(iAPlotData::DataType x, iAPlotData::DataType y)
{
	// find position to insert:
	size_t idx = 0;
	while (idx < m_values.size() && x > m_values[idx].first)
	{
		++idx;
	}
	m_values.insert(m_values.begin() + idx, std::make_pair(x, y));
	adaptBounds(m_xBounds, x);
	adaptBounds(m_yBounds, y);
}

size_t iAXYPlotData::nearestIdx(double dataX) const
{
	if (dataX < m_xBounds[0])
	{
		return 0;
	}
	if (dataX >= m_xBounds[1])
	{
		return valueCount() - 1;
	}
	// TODO: use binary search?
	for (size_t i = 1; i < m_values.size(); ++i)
	{
		if (m_values[i].first > dataX)
		{
			return i - 1;
		}
	}
	// shouldn't be necessary (since if dataX is >= last data value,
	//     xBounds check above would have triggered already)
	return valueCount() - 1;
}

QString iAXYPlotData::toolTipText(iAPlotData::DataType dataX) const
{
	size_t idx = nearestIdx(dataX);
	auto valueX = xValue(idx);
	auto valueY = yValue(idx);
	return QString("%1: %2").arg(valueX).arg(valueY);
}

QSharedPointer<iAXYPlotData> iAXYPlotData::create(QString const& name, iAValueType type, size_t reservedSize)
{
	return QSharedPointer<iAXYPlotData>(new iAXYPlotData(name, type, reservedSize));
}

iAXYPlotData::iAXYPlotData(QString const& name, iAValueType type, size_t reservedSize) :
	iAPlotData(name, type),
	m_xBounds{std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()},
	m_yBounds{std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()}
{
	m_values.reserve(reservedSize);
}
