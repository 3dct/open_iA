/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
 
#include "pch.h"
#include "iAParamHistogramData.h"

#include "iAChartFilter.h"
#include "iAChartAttributeMapper.h"
#include "iAConsole.h"
#include "iAImageTreeLeaf.h"
#include "iAMathUtility.h"

#include <algorithm> // for std::fill

double iAParamHistogramData::MapValueToBin(double value) const
{
	if (m_log)
	{
		double minLog = std::floor(LogFunc(XBounds()[0]));
		double maxLog = std::ceil (LogFunc(XBounds()[1]));
		double valueLog = LogFunc(value);
		valueLog = clamp(minLog, maxLog, valueLog);
		return mapValue(
			minLog, maxLog,
			0.0, static_cast<double>(GetNumBin()),
			valueLog
		);
	}
	return mapValue(XBounds()[0], XBounds()[1], 0.0, static_cast<double>(GetNumBin()), value);
}

double iAParamHistogramData::MapBinToValue(double bin) const
{
	if (m_log)
	{
		double minLog = std::floor(LogFunc(XBounds()[0]));
		double maxLog = std::ceil (LogFunc(XBounds()[1]));
		double yLog = mapValue(
			0.0, static_cast<double>(m_numBin),
			minLog, maxLog,
			bin
			);
		return std::pow(LogBase, yLog);
	}
	return mapValue(0.0, static_cast<double>(GetNumBin()), XBounds()[0], XBounds()[1], bin);
}


void iAParamHistogramData::CountNodeBin(iAImageTreeLeaf const* leaf,
	QSharedPointer<iAParamHistogramData> data, int chartID,
	iAChartAttributeMapper const & chartAttrMap)
{
	if (!chartAttrMap.GetDatasetIDs(chartID).contains(leaf->GetDatasetID()))
	{
		return;
	}
	int attributeID = chartAttrMap.GetAttributeID(chartID, leaf->GetDatasetID());
	double value = leaf->GetAttribute(attributeID);
	data->AddValue(value);
}


void iAParamHistogramData::VisitNode(iAImageTreeNode const * node,
	QSharedPointer<iAParamHistogramData> data, int chartID,
	iAChartAttributeMapper const & chartAttrMap)
{
	VisitLeafs(node, [&](iAImageTreeLeaf const * leaf)
	{
		CountNodeBin(leaf, data, chartID, chartAttrMap);
	});
}


void iAParamHistogramData::VisitNode(iAImageTreeNode const * node,
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
		CountNodeBin(leaf, data, chartID, chartAttrMap);
	});
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


QSharedPointer<iAParamHistogramData> iAParamHistogramData::Create(iAImageTreeNode const * tree,
	int chartID,
	iAValueType rangeType,
	double min, double max, bool log,
	iAChartAttributeMapper const & chartAttrMap,
	int numBin)
{
	if (rangeType == iAValueType::Discrete || rangeType == iAValueType::Categorical)
	{
		max = max+1;
	}
	QSharedPointer<iAParamHistogramData> result(new iAParamHistogramData(numBin, min, max, log, rangeType));
	VisitNode(tree, result, chartID, chartAttrMap);
	return result;
}


QSharedPointer<iAParamHistogramData> iAParamHistogramData::Create(iAImageTreeNode const * tree,
	int chartID,
	iAValueType rangeType,
	double min, double max, bool log,
	iAChartAttributeMapper const & chartAttrMap,
	iAChartFilter const & attributeFilter,
	int numBin)
{
	if (rangeType == iAValueType::Discrete || rangeType == iAValueType::Categorical)
	{
		max = max+1;
	}
	QSharedPointer<iAParamHistogramData> result(new iAParamHistogramData(numBin, min, max, log, rangeType));
	VisitNode(tree, result, chartID, chartAttrMap, attributeFilter);
	if (attributeFilter.HasFilter(chartID))
	{
		result->SetMinX(result->MapValueToBin(attributeFilter.GetMin(chartID)));
		result->SetMaxX(result->MapValueToBin(attributeFilter.GetMax(chartID)));
	}
	return result;
}

iAParamHistogramData::iAParamHistogramData(size_t numBin, double min, double max, bool log, iAValueType rangeType):
	m_data(new DataType[numBin > 0 ? numBin : 1]),
	m_numBin(numBin > 0? numBin : 1),
	m_maxValue(std::numeric_limits<double>::lowest()),
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
	std::fill(m_data, m_data+numBin, 0.0);
	m_dataRange[0] = min;
	m_dataRange[1] = (min == max)? min+1: max;
	m_spacing = (max - min) / m_numBin;
}


void iAParamHistogramData::Reset()
{
	m_maxValue = std::numeric_limits<double>::lowest();
	std::fill(m_data, m_data + m_numBin, 0.0);
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

double const * iAParamHistogramData::XBounds() const
{
	return m_dataRange;
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


double iAParamHistogramData::GetMinX() const
{
	return m_minX;
}
double iAParamHistogramData::GetMaxX() const
{
	return m_maxX;
}
void iAParamHistogramData::SetMinX(double x)
{
	m_minX = x;
}
void iAParamHistogramData::SetMaxX(double x)
{
	m_maxX = x;
}

void iAParamHistogramData::AddValue(double value)
{
	int binIdx = clamp(0, static_cast<int>(m_numBin - 1), static_cast<int>(MapValueToBin(value)));
	m_data[binIdx]++;
	if (m_data[binIdx] > m_maxValue)
	{
		m_maxValue = m_data[binIdx];
	}
}
