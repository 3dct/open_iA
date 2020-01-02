/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "open_iA_Core_export.h"

typedef int iAVoxelIndexType;

//! Helper struct for storing 3D image coordinates.
struct open_iA_Core_API iAImageCoordinate
{
public:
	enum iAIndexOrdering
	{
		RowColDepMajor, // x, y(, z)
		ColRowDepMajor, // y, x(, z)
	};
	iAImageCoordinate();
	iAImageCoordinate(iAVoxelIndexType x, iAVoxelIndexType y, iAVoxelIndexType z);
	iAVoxelIndexType x, y, z;
};

bool operator==(iAImageCoordinate const & a, iAImageCoordinate const & b);

//! Utility class for converting (2D/)3D indices to a flat (1D) index 
class open_iA_Core_API iAImageCoordConverter
{
public:
	iAImageCoordConverter(iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth=1,
		iAImageCoordinate::iAIndexOrdering ordering=iAImageCoordinate::RowColDepMajor
	);
	iAImageCoordinate coordinatesFromIndex(iAVoxelIndexType index) const;
	iAVoxelIndexType indexFromCoordinates(iAImageCoordinate coords) const;
	iAVoxelIndexType vertexCount() const;
	static iAImageCoordinate coordinatesFromIndex(
		iAVoxelIndexType index,
		iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth,
		iAImageCoordinate::iAIndexOrdering ordering);
	static iAVoxelIndexType indexFromCoordinates(
		iAImageCoordinate coords,
		iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth,
		iAImageCoordinate::iAIndexOrdering ordering);
	iAVoxelIndexType width() const;
	iAVoxelIndexType height() const;
	iAVoxelIndexType depth() const;
private:
	iAVoxelIndexType m_width;
	iAVoxelIndexType m_height;
	iAVoxelIndexType m_depth;
	iAImageCoordinate::iAIndexOrdering m_ordering;
};
