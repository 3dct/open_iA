/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iAXYPlotData.h"

#include "iALog.h"

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
	// make sure elements can only be entered in an ordered way:
	if (m_values[m_values.size()-1].first >= x)
	{
		LOG(lvlWarn, QString("Values have to be entered with ascending x component"
			", but given x=%1 is not larger than previous x=%2!")
			.arg(x)
			.arg(m_values[m_values.size() - 1].first));
		return;
	}
	m_values.push_back(std::make_pair(x, y));
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

QSharedPointer<iAXYPlotData> iAXYPlotData::create(iAValueType type, size_t valueCount)
{
	return QSharedPointer<iAXYPlotData>(new iAXYPlotData(type, valueCount));
}

iAXYPlotData::iAXYPlotData(iAValueType type, size_t valueCount) :
	iAPlotData(type),
	m_values(valueCount, std::make_pair(0.0, 0.0)),
	m_xBounds{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()},
	m_yBounds{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()}
{
}
