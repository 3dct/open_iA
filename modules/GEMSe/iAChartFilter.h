// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAGEMSeConstants.h"

#include <iAImageCoordinate.h>

#include <QMap>

class iAChartAttributeMapper;
class iAImageTreeLeaf;

class iAChartFilter
{
public:
	void RemoveFilter(int chartID);
	void AddFilter(int chartID, double min, double max);
	bool Matches(iAImageTreeLeaf const * leaf, iAChartAttributeMapper const & chartAttrMap) const;
	bool MatchesAll() const;
	void Reset();
	bool HasFilter(int chartID) const;
	double GetMin(int chartID) const;
	double GetMax(int chartID) const;
private:
	QMap<int, std::pair<double, double> > m_filters;
};


typedef QVector<QPair<iAImageCoordinate, int> > iAResultFilter;

bool ResultFilterMatches(iAImageTreeLeaf const * leaf, iAResultFilter const & filter);
