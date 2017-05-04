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

#include "open_iA_Core_export.h"

typedef int iAVoxelIndexType;

/**
 * \brief Helper struct for containing 3D image coordinates
 */
struct open_iA_Core_API iAImageCoordinate
{
public:
	enum IndexOrdering
	{
		RowColDepMajor, // x, y(, z)
		ColRowDepMajor, // y, x(, z)
	};
	iAImageCoordinate();
	iAImageCoordinate(iAVoxelIndexType x, iAVoxelIndexType y, iAVoxelIndexType z);
	iAVoxelIndexType x, y, z;
};

bool operator==(iAImageCoordinate const & a, iAImageCoordinate const & b);

/**
 * \brief Conversion utilty class
 *
 * This class converts from a up to 3-dimensional image index (x, y, z) to a "flat" index
 */
class open_iA_Core_API iAImageCoordConverter
{
public:
	iAImageCoordConverter(iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth=1,
		iAImageCoordinate::IndexOrdering ordering=iAImageCoordinate::RowColDepMajor
	);
	iAImageCoordinate GetCoordinatesFromIndex(iAVoxelIndexType index) const;
	iAVoxelIndexType GetIndexFromCoordinates(iAImageCoordinate coords) const;
	iAVoxelIndexType GetVertexCount() const;
	static iAImageCoordinate GetCoordinatesFromIndex(
		iAVoxelIndexType index,
		iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth,
		iAImageCoordinate::IndexOrdering ordering);
	static iAVoxelIndexType GetIndexFromCoordinates(
		iAImageCoordinate coords,
		iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth,
		iAImageCoordinate::IndexOrdering ordering);
	iAVoxelIndexType GetWidth() const;
	iAVoxelIndexType GetHeight() const;
	iAVoxelIndexType GetDepth() const;
private:
	iAVoxelIndexType m_width;
	iAVoxelIndexType m_height;
	iAVoxelIndexType m_depth;
	iAImageCoordinate::IndexOrdering m_ordering;
};
