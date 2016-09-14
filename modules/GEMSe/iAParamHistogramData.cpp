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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "iAParamHistogramData.h"

#include "iAChartFilter.h"
#include "iAChartAttributeMapper.h"
#include "iAConsole.h"
#include "iAImageTree.h"
#include "iAMathUtility.h"

#include <algorithm> // for std::fill

double iAParamHistogramData::mapValueToBin(double value) const
{
	if (m_log)
	{
		double minLog = std::floor(LogFunc(GetDataRange(0)));
		double maxLog = std::ceil (LogFunc(GetDataRange(1)));
		double valueLog = LogFunc(value);
		valueLog = clamp(minLog, maxLog, valueLog);
		return mapValue(
			minLog, maxLog,
			0.0, static_cast<double>(GetNumBin()),
			valueLog
		);
	}
	return mapValue(GetDataRange(0), GetDataRange(1), 0.0, static_cast<double>(GetNumBin()), value);
}

double iAParamHistogramData::mapBinToValue(double bin) const
{
	if (m_log)
	{
		double minLog = std::floor(LogFunc(GetDataRange(0)));
		double maxLog = std::ceil (LogFunc(GetDataRange(1)));
		double yLog = mapValue(
			0.0, static_cast<double>(m_numBin),
			minLog, maxLog,
			bin
			);
		return std::pow(LogBase, yLog);
	}
	return mapValue(0.0, static_cast<double>(GetNumBin()), GetDataRange(0), GetDataRange(1), bin);
}


void iAParamHistogramData::CountNodeBin(iAImageClusterNode const * node,
	QSharedPointer<iAParamHistogramData> data, int chartID,
	iAChartAttributeMapper const & chartAttrMap)
{
	iAImageClusterLeaf* leaf = (iAImageClusterLeaf*)node;
	if (!chartAttrMap.GetDatasetIDs(chartID).contains(leaf->GetDatasetID()))
	{
		return;
	}
	int attributeID = chartAttrMap.GetAttributeID(chartID, leaf->GetDatasetID());
	double value = node->GetAttribute(attributeID);
	int binIdx = clamp(0, static_cast<int>(data->m_numBin-1), static_cast<int>(data->mapValueToBin(value)));
	data->m_data[binIdx]++;
	if (data->m_data[binIdx] > data->m_maxValue)
	{
		data->m_maxValue = data->m_data[binIdx];
	}
}


void iAParamHistogramData::VisitNode(iAImageClusterNode const * node,
	QSharedPointer<iAParamHistogramData> data, int chartID,
	iAChartAttributeMapper const & chartAttrMap)
{
	if (!node->IsLeaf())
	{
		for (int i=0; i<node->GetChildCount(); ++i)
		{
			VisitNode(node->GetChild(i).data(), data, chartID, chartAttrMap);
		}
	}
	else
	{
		CountNodeBin(node, data, chartID, chartAttrMap);
	}
}


void iAParamHistogramData::VisitNode(iAImageClusterNode const * node,
	QSharedPointer<iAParamHistogramData> data, int chartID,
	iAChartAttributeMapper const & chartAttrMap,
	iAChartFilter const & attributeFilter)
{
	if (!node->IsLeaf())
	{
		for (int i=0; i<node->GetChildCount(); ++i)
		{
			VisitNode(node->GetChild(i).data(), data, chartID, chartAttrMap, attributeFilter);
		}
	}
	else
	{
		iAImageClusterLeaf const * leaf = dynamic_cast<iAImageClusterLeaf const *> (node);
		assert(leaf);
		if (!attributeFilter.Matches(leaf, chartAttrMap))
		{
			return;
		}
		CountNodeBin(node, data, chartID, chartAttrMap);
	}
}


double iAParamHistogramData::GetBinStart(int binNr) const
{
	if (!m_log)
	{
		return iAAbstractDiagramRangedData::GetBinStart(binNr);
	}
	double minLog = std::floor(LogFunc(m_dataRange[0]));
	double maxLog = std::ceil (LogFunc(m_dataRange[1]));
	double valueLog = mapValue(0, static_cast<int>(m_numBin), minLog, maxLog, binNr);
	return std::pow(LogBase, valueLog);
}


QSharedPointer<iAParamHistogramData> iAParamHistogramData::Create(iAImageClusterNode const * tree,
	int chartID,
	iAValueType rangeType,
	double min, double max, bool log,
	iAChartAttributeMapper const & chartAttrMap)
{
	// maximum number of bins:
	//		- square root of number of values (https://en.wikipedia.org/wiki/Histogram#Number_of_bins_and_width)
	//      - adapting to width of histogram?
	//      - if discrete or categorical values: limit by range
	size_t maxBin = std::min(static_cast<size_t>(std::sqrt(tree->GetClusterSize())), HistogramBinCount);
	int numBin = (min == max)? 1 :
		(rangeType == iAValueType::Discrete || rangeType == iAValueType::Categorical) ?
		std::min(static_cast<size_t>(max-min+1), maxBin) :
		maxBin;
	if (rangeType == iAValueType::Discrete || rangeType == iAValueType::Categorical)
	{
		max = max+1;
	}
	QSharedPointer<iAParamHistogramData> result(new iAParamHistogramData(numBin, min, max, log, rangeType));
	VisitNode(tree, result, chartID, chartAttrMap);
	return result;
}


QSharedPointer<iAParamHistogramData> iAParamHistogramData::Create(iAImageClusterNode const * tree,
	int chartID,
	iAValueType rangeType,
	double min, double max, bool log,
	iAChartAttributeMapper const & chartAttrMap,
	iAChartFilter const & attributeFilter)
{
	int numBin = (min == max)? 1 :
		(rangeType == iAValueType::Discrete || rangeType == iAValueType::Categorical) ?
		std::min(static_cast<size_t>(max-min+1), HistogramBinCount) :
		HistogramBinCount;
	if (rangeType == iAValueType::Discrete || rangeType == iAValueType::Categorical)
	{
		max = max+1;
	}
	QSharedPointer<iAParamHistogramData> result(new iAParamHistogramData(numBin, min, max, log, rangeType));
	VisitNode(tree, result, chartID, chartAttrMap, attributeFilter);
	return result;
}

iAParamHistogramData::iAParamHistogramData(size_t numBin, double min, double max, bool log, iAValueType rangeType):
	m_data(new DataType[numBin > 0 ? numBin : 1]),
	m_numBin(numBin > 0? numBin : 1),
	m_maxValue(std::numeric_limits<double>::lowest()),
	m_spacing(1.0),
	m_rangeType(rangeType),
	m_log(log)
{
	assert(numBin > 0);
	assert(!m_log || min > 0);
	if (m_log && min <= 0)
	{
		DEBUG_LOG("Need to define minimum bigger than 0 for logarithmic scale!");
		min = 0.000001;
	}
	std::fill(m_data, m_data+numBin, 0.0);
	m_dataRange[0] = min;
	m_dataRange[1] = (min == max)? min+1: max;
	m_spacing = (max - min) / m_numBin;
}

iAParamHistogramData::~iAParamHistogramData()
{
	delete [] m_data;
}

iAParamHistogramData::DataType const * iAParamHistogramData::GetData() const
{
	return m_data;
}

size_t iAParamHistogramData::GetNumBin() const
{
	return m_numBin;
}

double iAParamHistogramData::GetSpacing() const
{
	return m_spacing;
}

double * iAParamHistogramData::GetDataRange()
{
	return m_dataRange;
}

double iAParamHistogramData::GetDataRange(int idx) const
{
	return m_dataRange[idx];
}

iAParamHistogramData::DataType iAParamHistogramData::GetMaxValue() const
{
	return m_maxValue;
}

iAValueType iAParamHistogramData::GetRangeType() const
{
	return m_rangeType;
}

bool iAParamHistogramData::IsLogarithmic() const
{
	return m_log;
}
