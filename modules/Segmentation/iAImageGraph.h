/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
	bool containsEdge(iAVoxelIndexType voxel1, iAVoxelIndexType voxel2);
	bool containsEdge(iAImageCoordinate voxel1, iAImageCoordinate voxel2);
	iAImageCoordConverter const & converter() const;
private:
	void addEdge(iAImageCoordinate voxel1, iAImageCoordinate voxel2);
	iAImageCoordConverter m_converter;
	QVector<iAEdgeType> m_edges;
};
