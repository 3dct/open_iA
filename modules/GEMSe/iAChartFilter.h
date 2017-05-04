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
#pragma once

#include "iAGEMSeConstants.h"
#include "iAImageCoordinate.h"

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