/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "graphstat.h"

GraphStat::GraphStat()
{ }

GraphStat::GraphStat(Graph* g)
	: m_graph{g}
{ }

void GraphStat::setGraph(Graph* g)
{
	m_graph = g;
}

bool GraphStat::isHeaderVertix(Graph::idType v)
{
	//const Graph::EdgesMap* edges = m_graph->getEdges();
	//for (EdgesIterator edgeIt = edges->begin(); edgeIt != edges->end(); edgeIt++) {
	//	if (edgeIt->second.vertTo == v) return false;
	//}
	//return true;
	return m_isHeaderVertex[v];
}

GraphStat::VerticesIDs GraphStat::getParentVertices(Graph::idType v) {
	//VerticesIDs parents;
	//const Graph::EdgesMap* edges = m_graph->getEdges();
	//for (EdgesIterator edgeIt = edges->begin(); edgeIt != edges->end(); edgeIt++) {
	//	if (edgeIt->second.vertTo == v)
	//		parents.push_back(edgeIt->second.vertFrom);
	//}
	//return parents;
	return m_parentVertices[v];
}

GraphStat::VerticesIDs GraphStat::getChildVertices(Graph::idType v) {
	//VerticesIDs childs;
	//const Graph::EdgesMap* edges = m_graph->getEdges();
	//for (EdgesIterator edgeIt = edges->begin(); edgeIt != edges->end(); edgeIt++) {
	//	if (edgeIt->second.vertFrom == v)
	//		childs.push_back(edgeIt->second.vertTo);
	//}
	//return childs;
	return m_childVertices[v];
}

int GraphStat::getMaxRank() {
	return m_maxRank;
}

void GraphStat::update()
{
	m_maxRank = 0;
	const Graph::VerticesMap* vertices = m_graph->getVertices();
	for (VerticesIterator vertIt = vertices->begin(); vertIt != vertices->end(); vertIt++) {
		int rank = vertIt->second.rank;
		if(rank > m_maxRank) m_maxRank = rank;
	}
	m_numVertices = vector<int>(m_maxRank+1, 0);
	for (VerticesIterator vertIt = vertices->begin(); vertIt != vertices->end(); vertIt++) {
		Graph::idType id = vertIt->first;
		int rank = vertIt->second.rank;
		m_numVertices[rank]++;

		m_isHeaderVertex[id] = true;
		VerticesIDs parents, childs;
		const Graph::EdgesMap* edges = m_graph->getEdges();
		for(EdgesIterator edgeIt = edges->begin(); edgeIt != edges->end(); edgeIt++) {
			if(edgeIt->second.vertTo == id)
				parents.push_back(edgeIt->second.vertTo);
			if(edgeIt->second.vertFrom == id)
				childs.push_back(edgeIt->second.vertTo);
			if(edgeIt->second.vertTo == id) m_isHeaderVertex[id] = false;
		} 
		m_parentVertices[id] = parents;
		m_childVertices[id] = childs;
	}
}

int GraphStat::getNumVerticesInRank(int rank) {
	return m_numVertices[rank];
}
