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
#include "iAParamHistogramData.h"

#include "iAChartFilter.h"
#include "iAChartAttributeMapper.h"
#include "iAImageTreeLeaf.h"

#include <iAConsole.h>
#include <iAMapperImpl.h>    // for LogFunc -> TODO: use iAMapper-derived classes here!
#include <iAMathUtility.h>

#include <algorithm> // for std::fill

double iAParamHistogramData::mapValueToBin(double value) const
{
	if (m_log)
	{
		double minLog = std::floor(LogFunc(xBounds()[0]));
		double maxLog = std::ceil (LogFunc(xBounds()[1]));
		double valueLog = LogFunc(value);
		valueLog = clamp(minLog, maxLog, valueLog);
		return mapValue(
			minLog, maxLog,
			0.0, static_cast<double>(numBin()),
			valueLog
		);
	}
	return mapValue(xBounds()[0], xBounds()[1], 0.0, static_cast<double>(numBin()), value);
}

double iAParamHistogramData::mapBinToValue(double bin) const
{
	if (m_log)
	{
		double minLog = std::floor(LogFunc(xBounds()[0]));
		double maxLog = std::ceil (LogFunc(xBounds()[1]));
		double yLog = mapValue(
			0.0, static_cast<double>(m_numBin),
			minLog, maxLog,
			bin
			);
		return std::pow(LogBase, yLog);
	}
	return mapValue(0.0, static_cast<double>(numBin()), xBounds()[0], xBounds()[1], bin);
}

void iAParamHistogramData::countNodeBin(iAImageTreeLeaf const* leaf,
	QSharedPointer<iAParamHistogramData> data, int chartID,
	iAChartAttributeMapper const & chartAttrMap)
{
	if (!chartAttrMap.GetDatasetIDs(chartID).contains(leaf->GetDatasetID()))
	{
		return;
	}
	int attributeID = chartAttrMap.GetAttributeID(chartID, leaf->GetDatasetID());
	double value = leaf->GetAttribute(attributeID);
	data->addValue(value);
}

void iAParamHistogramData::visitNode(iAImageTreeNode const * node,
	QSharedPointer<iAParamHistogramData> data, int chartID,
	iAChartAttributeMapper const & chartAttrMap)
{
	VisitLeafs(node, [&](iAImageTreeLeaf const * leaf)
	{
		countNodeBin(leaf, data, chartID, chartAttrMap);
	});
}

void iAParamHistogramData::visitNode(iAImageTreeNode const * node,
	QSharedPointer<iAParamHistogramData> data, int chartID,
	iAChartAttributeMapper const & chartAttrMap,
	iAChartFilter const & attributeFilter)
{
	VisitLeafs(node, [&](iAImageTreeLeaf const * leaf)
	{
		if (!attributeFilter.Matches(leaf, chartAttrMap))
		{
			return;
		}
		countNodeBin(leaf, data, chartID, chartAttrMap);
	});
}

double iAParamHistogramData::binStart(size_t binNr) const
{
	if (!m_log)
	{
		return iAPlotData::binStart(binNr);
	}
	double minLog = std::floor(LogFunc(m_xBounds[0]));
	double maxLog = std::ceil (LogFunc(m_xBounds[1]));
	double valueLog = mapValue(static_cast<size_t>(0), m_numBin, minLog, maxLog, binNr);
	return std::pow(LogBase, valueLog);
}

QSharedPointer<iAParamHistogramData> iAParamHistogramData::create(iAImageTreeNode const * tree,
	int chartID,
	iAValueType rangeType,
	double min, double max, bool log,
	iAChartAttributeMapper const & chartAttrMap,
	int numBin)
{
	QSharedPointer<iAParamHistogramData> result(new iAParamHistogramData(numBin, min, max, log, rangeType));
	visitNode(tree, result, chartID, chartAttrMap);
	return result;
}

QSharedPointer<iAParamHistogramData> iAParamHistogramData::create(iAImageTreeNode const * tree,
	int chartID,
	iAValueType rangeType,
	double min, double max, bool log,
	iAChartAttributeMapper const & chartAttrMap,
	iAChartFilter const & attributeFilter,
	int numBin)
{
	QSharedPointer<iAParamHistogramData> result(new iAParamHistogramData(numBin, min, max, log, rangeType));
	visitNode(tree, result, chartID, chartAttrMap, attributeFilter);
	if (attributeFilter.HasFilter(chartID))
	{
		result->setMinX(result->mapValueToBin(attributeFilter.GetMin(chartID)));
		result->setMaxX(result->mapValueToBin(attributeFilter.GetMax(chartID)));
	}
	return result;
}

iAParamHistogramData::iAParamHistogramData(size_t numBin, double min, double max, bool log, iAValueType rangeType) :
	m_data(new DataType[numBin > 0 ? numBin : 1]),
	m_numBin(numBin > 0 ? numBin : 1),
	m_spacing(1.0),
	m_rangeType(rangeType),
	m_log(log),
	m_minX(0),
	m_maxX(numBin)
{
	assert(numBin > 0);
	assert(!m_log || min > 0);
	if (m_log && min <= 0)
	{
		DEBUG_LOG("Need to define minimum bigger than 0 for logarithmic scale!");
		min = 0.000001;
	}
	reset();
	m_xBounds[0] = min;
	m_xBounds[1] = (min == max)? min+1: max;
	m_spacing = (max - min) / m_numBin;
}

void iAParamHistogramData::reset()
{
	m_yBounds[0] = 0;
	m_yBounds[1] = std::numeric_limits<double>::lowest();
	std::fill(m_data, m_data + m_numBin, 0.0);
}

iAParamHistogramData::~iAParamHistogramData()
{
	delete [] m_data;
}

iAParamHistogramData::DataType const * iAParamHistogramData::rawData() const
{
	return m_data;
}

size_t iAParamHistogramData::numBin() const
{
	return m_numBin;
}

double iAParamHistogramData::spacing() const
{
	return m_spacing;
}

double const * iAParamHistogramData::xBounds() const
{
	return m_xBounds;
}

iAParamHistogramData::DataType const * iAParamHistogramData::yBounds() const
{
	return m_yBounds;
}

iAValueType iAParamHistogramData::valueType() const
{
	return m_rangeType;
}

bool iAParamHistogramData::isLogarithmic() const
{
	return m_log;
}

double iAParamHistogramData::minX() const
{
	return m_minX;
}

double iAParamHistogramData::maxX() const
{
	return m_maxX;
}

void iAParamHistogramData::setMinX(double x)
{
	m_minX = x;
}

void iAParamHistogramData::setMaxX(double x)
{
	m_maxX = x;
}

void iAParamHistogramData::addValue(double value)
{
	int binIdx = clamp(0, static_cast<int>(m_numBin - 1), static_cast<int>(mapValueToBin(value)));
	m_data[binIdx]++;
	if (m_data[binIdx] > m_yBounds[1])
	{
		m_yBounds[1] = m_data[binIdx];
	}
}
