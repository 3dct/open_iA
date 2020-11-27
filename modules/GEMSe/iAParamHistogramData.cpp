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
#include "iAParamHistogramData.h"

#include "iAChartFilter.h"
#include "iAChartAttributeMapper.h"
#include "iAImageTreeLeaf.h"

#include <iALog.h>
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
			0.0, static_cast<double>(valueCount()),
			valueLog
		);
	}
	return mapValue(xBounds()[0], xBounds()[1], 0.0, static_cast<double>(valueCount()), value);
}

double iAParamHistogramData::mapBinToValue(double bin) const
{
	if (m_log)
	{
		double minLog = std::floor(LogFunc(xBounds()[0]));
		double maxLog = std::ceil (LogFunc(xBounds()[1]));
		double yLog = mapValue(
			0.0, static_cast<double>(valueCount()),
			minLog, maxLog,
			bin
			);
		return std::pow(LogBase, yLog);
	}
	return mapValue(0.0, static_cast<double>(valueCount()), xBounds()[0], xBounds()[1], bin);
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

double iAParamHistogramData::xValue(size_t idx) const
{
	if (!m_log)
	{
		return iAHistogramData::xValue(idx);
	}
	double minLog = std::floor(LogFunc(xBounds()[0]));
	double maxLog = std::ceil (LogFunc(xBounds()[1]));
	double valueLog = mapValue(static_cast<size_t>(0), valueCount(), minLog, maxLog, idx);
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
	return result;
}

iAParamHistogramData::iAParamHistogramData(size_t numBin, double minX, double maxX, bool log, iAValueType type) :
	iAHistogramData(m_log && minX <= 0 ? 0.000001 : minX,
		(minX == maxX) ? minX + 1 : maxX, numBin > 0 ? numBin : 1, type),
	m_log(log)
{
	assert(numBin > 0);
	assert(!m_log || minX > 0);
	if (m_log && minX <= 0)
	{
		LOG(lvlWarn, QString("Need to define minimum bigger than 0 for logarithmic scale, setting to %1!").arg(xBounds()[0]));
	}
	reset();
}

void iAParamHistogramData::reset()
{
	setYBounds(0, std::numeric_limits<double>::lowest());
	clear();
}

bool iAParamHistogramData::isLogarithmic() const
{
	return m_log;
}

void iAParamHistogramData::addValue(double value)
{
	int binIdx = clamp(0, static_cast<int>(valueCount() - 1), static_cast<int>(mapValueToBin(value)));
	setBin(binIdx, yValue(binIdx) + 1);
}
