// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGraphStat.h"

iAGraphStat::iAGraphStat()
{ }

iAGraphStat::iAGraphStat(iAGraph* g)
	: m_graph{g}
{ }

void iAGraphStat::setGraph(iAGraph* g)
{
	m_graph = g;
}

bool iAGraphStat::isHeaderVertix(iAGraph::idType v)
{
	//const iAGraph::EdgesMap* edges = m_graph->getEdges();
	//for (EdgesIterator edgeIt = edges->begin(); edgeIt != edges->end(); edgeIt++) {
	//	if (edgeIt->second.vertTo == v) return false;
	//}
	//return true;
	return m_isHeaderVertex[v];
}

iAGraphStat::VerticesIDs iAGraphStat::getParentVertices(iAGraph::idType v)
{
	//VerticesIDs parents;
	//const iAGraph::EdgesMap* edges = m_graph->getEdges();
	//for (EdgesIterator edgeIt = edges->begin(); edgeIt != edges->end(); edgeIt++) {
	//	if (edgeIt->second.vertTo == v)
	//		parents.push_back(edgeIt->second.vertFrom);
	//}
	//return parents;
	return m_parentVertices[v];
}

iAGraphStat::VerticesIDs iAGraphStat::getChildVertices(iAGraph::idType v)
{
	//VerticesIDs childs;
	//const iAGraph::EdgesMap* edges = m_graph->getEdges();
	//for (EdgesIterator edgeIt = edges->begin(); edgeIt != edges->end(); edgeIt++) {
	//	if (edgeIt->second.vertFrom == v)
	//		childs.push_back(edgeIt->second.vertTo);
	//}
	//return childs;
	return m_childVertices[v];
}

int iAGraphStat::getMaxRank()
{
	return m_maxRank;
}

void iAGraphStat::update()
{
	m_maxRank = 0;
	const iAGraph::VerticesMap* vertices = m_graph->getVertices();
	for (VerticesIterator vertIt = vertices->begin(); vertIt != vertices->end(); vertIt++)
	{
		int rank = vertIt->second.rank;
		if (rank > m_maxRank)
		{
			m_maxRank = rank;
		}
	}
	m_numVertices = std::vector<int>(m_maxRank+1, 0);
	for (VerticesIterator vertIt = vertices->begin(); vertIt != vertices->end(); vertIt++) {
		iAGraph::idType id = vertIt->first;
		int rank = vertIt->second.rank;
		m_numVertices[rank]++;

		m_isHeaderVertex[id] = true;
		VerticesIDs parents, childs;
		const iAGraph::EdgesMap* edges = m_graph->getEdges();
		for(EdgesIterator edgeIt = edges->begin(); edgeIt != edges->end(); edgeIt++)
		{
			if (edgeIt->second.vertTo == id)
			{
				parents.push_back(edgeIt->second.vertTo);
			}
			if (edgeIt->second.vertFrom == id)
			{
				childs.push_back(edgeIt->second.vertTo);
			}
			if (edgeIt->second.vertTo == id)
			{
				m_isHeaderVertex[id] = false;
			}
		}
		m_parentVertices[id] = parents;
		m_childVertices[id] = childs;
	}
}

int iAGraphStat::getNumVerticesInRank(int rank)
{
	return m_numVertices[rank];
}
