// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

// why is this not unsigned int?
typedef int iAVoxelIndexType;

//! Helper for storing 3D image coordinates.
//! @todo Maybe use iAVec3i instead?
class iAbase_API iAImageCoordinate
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
//! @todo maybe merge with stuff from DynamicVolumeLines?
class iAbase_API iAImageCoordConverter
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
