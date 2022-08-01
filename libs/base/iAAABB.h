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
#pragma once

// TODO: re-use in FIAKER/FiberSA(/DreamCaster?)
#include <iAVec3.h>
class iAAABB
{
public:
	iAAABB()
	{
		box[0].fill(std::numeric_limits<double>::max());
		box[1].fill(std::numeric_limits<double>::lowest());
	}
	iAAABB(double const* b) : box{iAVec3d(b[0], b[2], b[4]), iAVec3d(b[1], b[3], b[5])}
	{
	}
	void addPointToBox(iAVec3d const& pt)
	{
		for (int i = 0; i < 3; ++i)
		{
			box[0][i] = std::min(box[0][i], pt[i]);
			box[1][i] = std::max(box[1][i], pt[i]);
		}
	}
	void merge(iAAABB const& other)
	{
		addPointToBox(other.topLeft());
		addPointToBox(other.bottomRight());
	}
	iAVec3d const& topLeft() const
	{
		return box[0];
	}
	iAVec3d const& bottomRight() const
	{
		return box[1];
	}

private:
	std::array<iAVec3d, 2> box;
};

QString toStr(iAAABB const& box)
{
	return QString("%1, %2; %3, %4").arg(box.topLeft().x()).arg(box.topLeft().y()).arg(box.bottomRight().x()).arg(box.bottomRight().y());
}