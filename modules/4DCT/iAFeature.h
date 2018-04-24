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
#pragma once

#include "iAVec3.h"
// std
#include <ostream>
#include <istream>

struct iAFeature
{
public:
					iAFeature();
	virtual			~iAFeature();
	int				id;
	int				parentId;
	int				childId;
	double			volume;
	Vec3d			centroid;
	Vec3d			eigenvalues;
	Vec3d			eigenvectors[3];
	Vec3d			axesLength;
	double			bb[6];			// bounding box
	double			bbVolume;
	Vec3d			bbSize;
	Vec3d			obbVertices[8];	// oriented bounding box
	double			obbVolume;
	Vec3d			obbSize;
};

std::ostream& operator<<(std::ostream& os, const iAFeature& obj);
bool operator>>(std::istream& is, iAFeature& obj);