// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAChartAttributeMapper.h"

#include <cassert>

int iAChartAttributeMapper::GetChartID(int datasetID, int attributeID) const
{
	return m_attributeChartMap[std::make_pair(datasetID, attributeID)];
}

QList<int> iAChartAttributeMapper::GetDatasetIDs(int chartID) const
{
	return m_chartAttributeMap[chartID].keys();
}

int iAChartAttributeMapper::GetAttributeID(int chartID, int datasetID) const
{
	assert(m_chartAttributeMap[chartID].contains(datasetID));
	return m_chartAttributeMap[chartID][datasetID];
}

int iAChartAttributeMapper::GetAttribCount(int datasetID) const
{
	return m_datasetAttribCount[datasetID];
}

void iAChartAttributeMapper::Add(int datasetID, int attributeID, int chartID)
{
	if (!m_chartAttributeMap.contains(chartID))
	{
		QMap<int, int> datatsetAttributeMap;
		datatsetAttributeMap.insert(datasetID, attributeID);
		m_chartAttributeMap.insert(chartID, datatsetAttributeMap);
	}
	else
	{
		m_chartAttributeMap[chartID][datasetID] = attributeID;
	}
	m_attributeChartMap.insert(std::make_pair(datasetID, attributeID), chartID);
	m_datasetAttribCount[datasetID] = std::max(
		m_datasetAttribCount.contains(datasetID) ? m_datasetAttribCount[datasetID] : 0,
		attributeID + 1
	);
}

void iAChartAttributeMapper::Clear()
{
	m_attributeChartMap.clear();
	m_chartAttributeMap.clear();
	m_datasetAttribCount.clear();
}
