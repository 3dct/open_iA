// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGraph.h"

iAGraph::iAGraph()
	: m_curInd{0}
{ }

iAGraph::idType iAGraph::addVertex(Vertex vert) {
	idType id = getNewID();
	m_vetices[id] = vert;
	return id;
}

iAGraph::idType iAGraph::addEdge(idType v1, idType v2) {
	idType id = getNewID();
	m_edges[id] = Edge(v1, v2);
	return id;
}

iAGraph::idType iAGraph::getNewID() {
	return m_curInd++;
}

iAGraph::VerticesMap* iAGraph::getVertices()
{
	return &m_vetices;
}

iAGraph::EdgesMap* iAGraph::getEdges()
{
	return &m_edges;
}
