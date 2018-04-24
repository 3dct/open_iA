/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

