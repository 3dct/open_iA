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
#include "iAChartFilter.h"

#include "iAChartAttributeMapper.h"
#include "iAImageTreeLeaf.h"


void iAChartFilter::RemoveFilter(int chartID)
{
	m_filters.remove(chartID);
}

void iAChartFilter::Reset()
{
	m_filters.clear();
}

void iAChartFilter::AddFilter(int chartID, double min, double max)
{
	m_filters.insert(chartID, std::make_pair(min, max));
}

bool iAChartFilter::Matches(iAImageTreeLeaf const * leaf, iAChartAttributeMapper const & chartAttrMap) const
{
	QList<int> chartIDs = m_filters.keys();
	for (int chartID: chartIDs)
	{
		if (!chartAttrMap.GetDatasetIDs(chartID).contains(leaf->GetDatasetID()))
		{	// filter doesn't apply for this leaf, so don't filter it
			return true;
		}
		int attributeID = chartAttrMap.GetAttributeID(chartID, leaf->GetDatasetID());
		double value = leaf->GetAttribute(attributeID);
		std::pair<double, double> const & range = m_filters[chartID];
		if (value < range.first || value > range.second)
		{
			return false;
		}
	}
	return true;
}

bool iAChartFilter::MatchesAll() const
{
	return m_filters.size() == 0;
}
