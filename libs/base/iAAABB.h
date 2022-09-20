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
#include "iAbase_export.h"

#include <iAVec3.h>

#include <QString>

#include <array>

class iAbase_API iAAABB
{
public:
	iAAABB();
	iAAABB(double const* b);
	//! add a point that should fit into the bounding box; if the current box does not contain this point, it is enlarged
	void addPointToBox(iAVec3d const& pt);
	//! merge another bounding box to this one, enlarging it if necessary
	void merge(iAAABB const& other);
	//! return true if the given other bounding box has intersecting space with this
	bool intersects(iAAABB const& other) const;
	iAVec3d const& minCorner() const;
	iAVec3d const& maxCorner() const;

private:
	std::array<iAVec3d, 2> box;
};

iAbase_API QString toStr(iAAABB const& box);