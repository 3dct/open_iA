#ifndef GRAPHSTAT_H
#define GRAPHSTAT_H

#include "graph.h"

#include <vector>
#include <unordered_map>

using namespace std;

class GraphStat
{
public:
	typedef vector<Graph::idType>	VerticesIDs;

						GraphStat();
						GraphStat(Graph* g);
	void				setGraph(Graph* g);
	void				update();
	bool				isHeaderVertix(Graph::idType v);
	VerticesIDs			getParentVertices(Graph::idType v);
	VerticesIDs			getChildVertices(Graph::idType v);
	int					getMaxRank();
	int					getNumVerticesInRank(int rank);


private:
	typedef Graph::EdgesMap::const_iterator		EdgesIterator;
	typedef Graph::VerticesMap::const_iterator	VerticesIterator;

	Graph*	m_graph;
	int		m_maxRank;
	unordered_map<Graph::idType, VerticesIDs>	m_parentVertices;
	unordered_map<Graph::idType, VerticesIDs>	m_childVertices;
	unordered_map<Graph::idType, bool>			m_isHeaderVertex;
	vector<int>									m_numVertices;
};

#endif // GRAPHSTAT_H