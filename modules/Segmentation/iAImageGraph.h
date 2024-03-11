// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageGraphTypes.h"

#include <iAImageCoordinate.h>

#include <vector>


class WeightCalculator
{
public:
	iAEdgeWeightType GetWeight(iAImageCoordinate const & point1, iAImageCoordinate const & point2);
};


//! Builds a graph for an image.
//! The image is specified via the given dimensions, where each pixel/voxel is
//! representing a vertex, and neighbouring pixels / voxels are connected via edges
//! (where neighbouring is at the moment defined as von-Neumann-neighbourhood, i.e.
//! those pixels with a Manhattan distance of 1).
class iAImageGraph
{
public:
	enum NeighbourhoodType
	{
		nbhVonNeumann,	//  6-neighbourhood
		nbhMoore		// 23-neighbourhood
	};
	iAImageGraph(iAVoxelIndexType width,
		iAVoxelIndexType height,
		iAVoxelIndexType depth=1,
		iAImageCoordinate::iAIndexOrdering indexOrdering = iAImageCoordinate::RowColDepMajor,
		NeighbourhoodType neighbourhoodType = nbhVonNeumann
	);

	iAEdgeIndexType edgeCount() const;
	iAEdgeType const & edge(iAEdgeIndexType idx) const;
	bool containsEdge(iAFlatIndexType voxel1, iAFlatIndexType voxel2);
	bool containsEdge(iAImageCoordinate voxel1, iAImageCoordinate voxel2);
	iAImageCoordConverter const & converter() const;
private:
	void addEdge(iAImageCoordinate voxel1, iAImageCoordinate voxel2);
	iAImageCoordConverter m_converter;
	QVector<iAEdgeType> m_edges;
};
