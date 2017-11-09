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

#include <cstddef>    // for size_t (in linux)

class iAImageInfo
{
public:
	iAImageInfo():
		m_voxelCount(0), m_min(0), m_max(0), m_mean(0), m_stdDev(0) {}
	iAImageInfo(size_t voxelCount, double min, double max, double mean, double stdDev) :
		m_voxelCount(voxelCount), m_min(min), m_max(max), m_mean(mean), m_stdDev(stdDev) {}
	size_t VoxelCount() const { return m_voxelCount; }
	double Min() const { return m_min; }
	double Max() const { return m_max; }
	double Mean() const { return m_mean; }
	double StandardDeviation() const { return m_stdDev; }
private:
	size_t m_voxelCount;
	double m_min, m_max, m_mean, m_stdDev;
};
