/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iAAABB.h"

#include <limits>

iAAABB::iAAABB()
{
	box[0].fill(std::numeric_limits<double>::max());
	box[1].fill(std::numeric_limits<double>::lowest());
}

iAAABB::iAAABB(double const* b) : box{ iAVec3d(b[0], b[2], b[4]), iAVec3d(b[1], b[3], b[5]) }
{
}

void iAAABB::addPointToBox(iAVec3d const& pt)
{
	for (int i = 0; i < 3; ++i)
	{
		box[0][i] = std::min(box[0][i], pt[i]);
		box[1][i] = std::max(box[1][i], pt[i]);
	}
}

void iAAABB::merge(iAAABB const& other)
{
	addPointToBox(other.minCorner());
	addPointToBox(other.maxCorner());
}

bool iAAABB::contains(iAVec3d const& pt) const
{
	return
		minCorner().x() <= pt.x() && pt.x() <= maxCorner().x() &&
		minCorner().y() <= pt.y() && pt.y() <= maxCorner().y() &&
		minCorner().z() <= pt.z() && pt.z() <= maxCorner().z();
}

bool iAAABB::intersects(iAAABB const& other) const
{
	return
		maxCorner().x() > other.minCorner().x() && other.maxCorner().x() > minCorner().x() &&
		maxCorner().y() > other.minCorner().y() && other.maxCorner().y() > minCorner().y() &&
		maxCorner().z() > other.minCorner().z() && other.maxCorner().z() > minCorner().z();
}

iAVec3d const& iAAABB::minCorner() const
{
	return box[0];
}

iAVec3d const& iAAABB::maxCorner() const
{
	return box[1];
}

QString toStr(iAAABB const& box)
{
	return QString("%1, %2; %3, %4").arg(box.minCorner().x()).arg(box.minCorner().y()).arg(box.maxCorner().x()).arg(box.maxCorner().y());
}
