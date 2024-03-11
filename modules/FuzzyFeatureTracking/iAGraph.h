// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <map>

class iAGraph
{
public:
	typedef long	idType;

	struct Vertex {
		Vertex()
			: id{0}
			, rank{0}
		{ }
		Vertex(int rank, float posX, float posY)
			: id{0}
			, rank{rank}
			, posX{posX}
			, posY{posY}
		{ }

		idType	id;
		int		rank;
		float	posX;
		float	posY;
	};

	struct Edge
	{
		Edge()
			: vertFrom{0}
			, vertTo{0}
		{ }
		Edge(idType vertFrom, idType vertTo)
			: vertFrom{vertFrom}
			, vertTo{vertTo}
		{ }
		// edge has a direction from vertex 1 to vertex 2
		idType vertFrom, vertTo;
	};

	typedef std::map<idType, Vertex> VerticesMap;
	typedef std::map<idType, Edge> EdgesMap;

	iAGraph();
	idType addVertex(Vertex vert);
	idType addEdge(idType v1, idType v2);
	VerticesMap* getVertices();
	EdgesMap* getEdges();

private:
	VerticesMap m_vetices;
	EdgesMap m_edges;
	idType m_curInd;

	idType getNewID();
};
