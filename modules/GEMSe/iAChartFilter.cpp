// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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


bool iAChartFilter::HasFilter(int chartID) const
{
	return m_filters.contains(chartID);
}

double iAChartFilter::GetMin(int chartID) const
{
	return HasFilter(chartID) ? m_filters[chartID].first : -1;
}

double iAChartFilter::GetMax(int chartID) const
{
	return HasFilter(chartID) ? m_filters[chartID].second : -1;
}

bool ResultFilterMatches(iAImageTreeLeaf const * leaf, iAResultFilter const & filter)
{
	LabelImageType* img = dynamic_cast<LabelImageType*>(leaf->GetLargeImage().GetPointer());
	for (auto filterEntry: filter)
	{
		itk::Index<3> idx;
		idx[0] = filterEntry.first.x;
		idx[1] = filterEntry.first.y;
		idx[2] = filterEntry.first.z;
		if (img->GetPixel(idx) != filterEntry.second)
		{
			leaf->DiscardDetails();
			return false;
		}
	}
	leaf->DiscardDetails();
	return true;
}
