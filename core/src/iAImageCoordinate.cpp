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
#include "iAImageCoordinate.h"

#include <cassert>

iAImageCoordinate::iAImageCoordinate()
{}

iAImageCoordinate::iAImageCoordinate(iAVoxelIndexType x, iAVoxelIndexType y, iAVoxelIndexType z) : x(x), y(y), z(z)
{}


bool operator==(iAImageCoordinate const & a, iAImageCoordinate const & b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}


iAImageCoordConverter::iAImageCoordConverter(
	iAVoxelIndexType width, iAVoxelIndexType height, iAVoxelIndexType depth, iAImageCoordinate::iAIndexOrdering ordering):
		m_width(width),
		m_height(height),
		m_depth(depth),
		m_ordering(ordering)
{
	assert(m_width >= 0 && m_height >= 0 && m_depth >= 0);
}

iAVoxelIndexType iAImageCoordConverter::vertexCount() const
{
	return m_width*m_height*m_depth;
}

iAImageCoordinate iAImageCoordConverter::coordinatesFromIndex(iAVoxelIndexType index) const
{
	return iAImageCoordConverter::coordinatesFromIndex(index, m_width, m_height, m_depth, m_ordering);
}

iAVoxelIndexType iAImageCoordConverter::indexFromCoordinates(iAImageCoordinate coords) const
{
	return iAImageCoordConverter::indexFromCoordinates(coords, m_width, m_height, m_depth, m_ordering);
}

iAImageCoordinate iAImageCoordConverter::coordinatesFromIndex(
		iAVoxelIndexType index,
		iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType /*depth*/,
		iAImageCoordinate::iAIndexOrdering ordering)
{
	iAImageCoordinate result;
	result.z = index / (width*height);
	switch (ordering)
	{
		default:
		case iAImageCoordinate::RowColDepMajor:
			result.y = (index / width) % height;
			result.x = index % width;
			break;
		case iAImageCoordinate::ColRowDepMajor:
			result.x = (index / height) % width;
			result.y = index % height;
	}
	return result;
}

iAVoxelIndexType iAImageCoordConverter::indexFromCoordinates(
		iAImageCoordinate coords,
		iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType
#ifndef NDEBUG // to silence compiler warning about unused parameter
			depth
#endif
		,
		iAImageCoordinate::iAIndexOrdering ordering)
{
	assert(coords.x >= 0 && coords.x < width &&
		coords.y >= 0 && coords.y < height &&
		coords.z >= 0 && coords.z < depth);
	switch (ordering)
	{
		default:
		case iAImageCoordinate::RowColDepMajor:
			return coords.z * (width*height) + coords.y*width + coords.x;
		case iAImageCoordinate::ColRowDepMajor:
			return coords.z * (width*height) + coords.x*height + coords.y;
	}
}


iAVoxelIndexType iAImageCoordConverter::width() const
{
	return m_width;
}

iAVoxelIndexType iAImageCoordConverter::height() const
{
	return m_height;
}

iAVoxelIndexType iAImageCoordConverter::depth() const
{
	return m_depth;
}
