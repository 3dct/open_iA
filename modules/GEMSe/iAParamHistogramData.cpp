// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	std::shared_ptr<iAParamHistogramData> data, int chartID,
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
	std::shared_ptr<iAParamHistogramData> data, int chartID,
	iAChartAttributeMapper const & chartAttrMap)
{
	VisitLeafs(node, [&](iAImageTreeLeaf const * leaf)
	{
		countNodeBin(leaf, data, chartID, chartAttrMap);
	});
}

void iAParamHistogramData::visitNode(iAImageTreeNode const * node,
	std::shared_ptr<iAParamHistogramData> data, int chartID,
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

std::shared_ptr<iAParamHistogramData> iAParamHistogramData::create(iAImageTreeNode const * tree,
	int chartID,
	iAValueType rangeType,
	double min, double max, bool log,
	iAChartAttributeMapper const & chartAttrMap,
	size_t numBin)
{
	auto result = std::make_shared<iAParamHistogramData>(numBin, min, max, log, rangeType);
	visitNode(tree, result, chartID, chartAttrMap);
	return result;
}

std::shared_ptr<iAParamHistogramData> iAParamHistogramData::create(iAImageTreeNode const * tree,
	int chartID,
	iAValueType rangeType,
	double min, double max, bool log,
	iAChartAttributeMapper const & chartAttrMap,
	iAChartFilter const & attributeFilter,
	size_t numBin)
{
	auto result = std::make_shared<iAParamHistogramData>(numBin, min, max, log, rangeType);
	visitNode(tree, result, chartID, chartAttrMap, attributeFilter);
	return result;
}

iAParamHistogramData::iAParamHistogramData(size_t numBin, double minX, double maxX, bool log, iAValueType type) :
	iAHistogramData("Frequency", type, log && minX <= 0 ? 0.000001 : minX,
		(minX == maxX) ? minX + 1 : maxX, numBin > 0 ? numBin : 1),
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
