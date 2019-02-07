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
#include "iAImageGraph.h"

namespace
{
	iAEdgeIndexType GetNumberOfEdges(iAVoxelIndexType width, iAVoxelIndexType height, iAVoxelIndexType depth,
		iAImageGraph::NeighbourhoodType neighbourhoodType)
	{
		switch (neighbourhoodType)
		{
			case iAImageGraph::nbhMoore:
				return 13*width*height*depth - 9*(depth*height+depth*width+height*width) + 6*(depth+height+width) - 4;
			default:
			case iAImageGraph::nbhVonNeumann:
				return 3*width*height*depth - height*width - height*depth - width*depth;

		}
	}
}

// iAImageGraph

iAImageGraph::iAImageGraph(iAVoxelIndexType width, iAVoxelIndexType height, iAVoxelIndexType depth,
		iAImageCoordinate::IndexOrdering indexOrdering,
		NeighbourhoodType neighbourhoodType
	):
		m_converter(width, height, depth, indexOrdering)
{
	m_edges.reserve(GetNumberOfEdges(width, height, depth, neighbourhoodType));
	iAVoxelIndexType N=width*height*depth;

	// edges are bi-directional; here we will always consider only one "working direction";
	// i.e. we connect vertices only downward (bidirectionality automatically connects the
	// lower vertex up!)

	for (iAVoxelIndexType i=0; i<N-1; ++i)
	{
		iAImageCoordinate coord1 = m_converter.GetCoordinatesFromIndex(i);

		// connect to right neighbour
		if (coord1.x < width-1)
		{
			iAImageCoordinate coord2(coord1);
			coord2.x += 1;
			AddEdge(coord1, coord2);
		}

		// connect to lower neighbour
		if (coord1.y < height-1)
		{
			iAImageCoordinate coord2(coord1);
			coord2.y += 1;
			AddEdge(coord1, coord2);
		}
		// connect to front/back neighbour (if depth == 1, then nothing will be added here)
		if (coord1.z < depth-1)
		{
			iAImageCoordinate coord2(coord1);
			coord2.z += 1;
			AddEdge(coord1, coord2);
		}
		if (neighbourhoodType == nbhMoore)
		{
			// add diagonal edges:
			if (coord1.x < width-1 && coord1.y < height-1)
			{
				iAImageCoordinate coord2_1(coord1);
				coord2_1.x += 1;
				coord2_1.y += 1;
				AddEdge(coord1, coord2_1);

				iAImageCoordinate coord1_2(coord1);
				coord1_2.x += 1;
				iAImageCoordinate coord2_2(coord1);
				coord2_2.y += 1;
				AddEdge(coord1_2, coord2_2);
			}
			if (coord1.x < width-1 && coord1.z < depth-1)
			{
				iAImageCoordinate coord2_1(coord1);
				coord2_1.x += 1;
				coord2_1.z += 1;
				AddEdge(coord1, coord2_1);

				iAImageCoordinate coord1_2(coord1);
				coord1_2.x += 1;
				iAImageCoordinate coord2_2(coord1);
				coord2_2.z += 1;
				AddEdge(coord1_2, coord2_2);
			}
			if (coord1.y < height-1 && coord1.z < depth-1)
			{
				iAImageCoordinate coord2_1(coord1);
				coord2_1.y += 1;
				coord2_1.z += 1;
				AddEdge(coord1, coord2_1);

				iAImageCoordinate coord1_2(coord1);
				coord1_2.y += 1;
				iAImageCoordinate coord2_2(coord1);
				coord2_2.z += 1;
				AddEdge(coord1_2, coord2_2);
			}

			if (coord1.x < width-1 && coord1.y < height-1 && coord1.z < depth-1)
			{
				iAImageCoordinate coord2_1(coord1);
				coord2_1.x += 1;
				coord2_1.y += 1;
				coord2_1.z += 1;
				AddEdge(coord1, coord2_1);

				iAImageCoordinate coord1_2(coord1);
				coord1_2.x += 1;
				iAImageCoordinate coord2_2(coord1);
				coord2_2.y += 1;
				coord2_2.z += 1;
				AddEdge(coord1_2, coord2_2);

				iAImageCoordinate coord1_3(coord1);
				coord1_3.y += 1;
				iAImageCoordinate coord2_3(coord1);
				coord2_3.x += 1;
				coord2_3.z += 1;
				AddEdge(coord1_3, coord2_3);

				iAImageCoordinate coord1_4(coord1);
				coord1_4.z += 1;
				iAImageCoordinate coord2_4(coord1);
				coord2_4.x += 1;
				coord2_4.y += 1;
				AddEdge(coord1_4, coord2_4);
			}
		}
	}
}

bool iAImageGraph::ContainsEdge(iAVoxelIndexType voxel1, iAVoxelIndexType voxel2)
{
	for (iAEdgeIndexType i=0; i<m_edges.size(); ++i)
	{
		// edges are bi-directional
		if ( (m_edges[i].first == voxel1 && m_edges[i].second == voxel2) ||
			 (m_edges[i].second == voxel1 && m_edges[i].first == voxel2))
		{
			return true;
		}
	}
	return false;
}

bool iAImageGraph::ContainsEdge(iAImageCoordinate voxel1, iAImageCoordinate voxel2)
{
	iAVoxelIndexType idx1 = m_converter.GetIndexFromCoordinates(voxel1);
	iAVoxelIndexType idx2 = m_converter.GetIndexFromCoordinates(voxel2);
	return ContainsEdge(idx1, idx2);
}

iAEdgeIndexType iAImageGraph::GetEdgeCount() const
{
	return m_edges.size();
}

iAEdgeType const & iAImageGraph::GetEdge(iAEdgeIndexType idx) const
{
	return m_edges[idx];
}

void iAImageGraph::AddEdge(iAImageCoordinate voxel1, iAImageCoordinate voxel2)
{
	iAVoxelIndexType idx1 = m_converter.GetIndexFromCoordinates(voxel1);
	iAVoxelIndexType idx2 = m_converter.GetIndexFromCoordinates(voxel2);
	/*
	assert (!ContainsEdge(idx1, idx2));
	assert (!ContainsEdge(idx2, idx1));
	*/
	//DebugOut() << "Adding edge #"<<idx1<<"(x="<<voxel1.x<<",y="<<voxel1.y<<",z="<<voxel1.z<<") -> #"<<
	//	                           idx2<<"(x="<<voxel2.x<<",y="<<voxel2.y<<",z="<<voxel2.z<<")" << std::endl;
	m_edges.push_back(std::make_pair(idx1, idx2));
}

iAImageCoordConverter const & iAImageGraph::GetConverter() const
{
	return m_converter;
}