// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAGraph.h"

#include <vector>
#include <unordered_map>

class iAGraphStat
{
public:
	typedef std::vector<iAGraph::idType>	VerticesIDs;

	iAGraphStat();
	iAGraphStat(iAGraph* g);
	void setGraph(iAGraph* g);
	void update();
	bool isHeaderVertix(iAGraph::idType v);
	VerticesIDs getParentVertices(iAGraph::idType v);
	VerticesIDs getChildVertices(iAGraph::idType v);
	int getMaxRank();
	int getNumVerticesInRank(int rank);

private:
	typedef iAGraph::EdgesMap::const_iterator    EdgesIterator;
	typedef iAGraph::VerticesMap::const_iterator VerticesIterator;

	iAGraph* m_graph;
	int m_maxRank;
	std::unordered_map<iAGraph::idType, VerticesIDs> m_parentVertices;
	std::unordered_map<iAGraph::idType, VerticesIDs> m_childVertices;
	std::unordered_map<iAGraph::idType, bool> m_isHeaderVertex;
	std::vector<int> m_numVertices;
};
