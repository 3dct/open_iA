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
