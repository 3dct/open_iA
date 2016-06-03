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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAGraphDrawer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>
#include <iostream>

//#define DEBUG
#define DEFAULT_MAX_ORDERING_ITERATION 24
#define DEFAULT_MAX_POSITIONING_ITERATION 8

idType iAGraphDrawer::m_uniqueIndex = 0;

bool compare(pair<idType, graphVertex*> const & v1, pair<idType, graphVertex*> const & v2) {
	return (v1.second->rank < v2.second->rank);
}

bool compareMedians(pair<idType, float> const & v1, pair<idType, float> const & v2) {
	return (v1.second < v2.second);
}

void swapVertices(vector<idType>* rank, int index1, int index2) {
	assert(rank);
	assert(index1 < rank->size());
	assert(index2 < rank->size());

	idType val = (*rank)[index1];
	(*rank)[index1] = (*rank)[index2];
	(*rank)[index2] = val;
}

class iAGraphDrawerImpl
{
public:
	iAGraphDrawerImpl() :
		m_maxOrderingIterations(DEFAULT_MAX_ORDERING_ITERATION),
		m_maxPositioningIterations(DEFAULT_MAX_POSITIONING_ITERATION),
		m_order(0)
	{}
	verticesMap		m_vertices;
	edgesMap		m_edges;
	int				m_maxOrderingIterations;
	int				m_maxPositioningIterations;
	orderType		m_order;
};


iAGraphDrawer::iAGraphDrawer():
	m_pImpl(new iAGraphDrawerImpl)
{}

iAGraphDrawer::~iAGraphDrawer() {
	for (verticesMap::iterator vertIt = m_pImpl->m_vertices.begin(); vertIt != m_pImpl->m_vertices.end(); vertIt++) {
		delete vertIt->second;
	}
	for (edgesMap::iterator edgeIt = m_pImpl->m_edges.begin(); edgeIt != m_pImpl->m_edges.end(); edgeIt++) {
		delete edgeIt->second;
	}
	delete m_pImpl;
}

idType iAGraphDrawer::getNewUniqueIndex() {
	return m_uniqueIndex++;
}

/*
=========================================
Modifiers
=========================================
*/

idType iAGraphDrawer::addVertex() {
	graphVertex* vert = new graphVertex();
	idType id = this->getNewUniqueIndex();
	m_pImpl->m_vertices.insert(pair<idType, graphVertex*>(id, vert));
	return id;
}

idType iAGraphDrawer::addVertex(graphVertex vert) {
	idType v = this->addVertex();
	this->modifyVertex(v, vert);
	return v;
}

bool iAGraphDrawer::modifyVertex(idType vertId, graphVertex value) {
	if (m_pImpl->m_vertices[vertId] == NULL) return false;
	*m_pImpl->m_vertices[vertId] = value;
	return true;
}

idType iAGraphDrawer::addEdge(idType vert1, idType vert2) {
	graphEdge* edge = new graphEdge();
	edge->vert1 = vert1;
	edge->vert2 = vert2;
	idType id = this->getNewUniqueIndex();
	m_pImpl->m_edges.insert(pair<idType, graphEdge*>(id, edge));
	return id;
}

/*
===============
Operations
===============
*/

bool iAGraphDrawer::isHeaderVerticle(idType vert) {
	for (edgesMap::iterator edgeIt = m_pImpl->m_edges.begin(); edgeIt != m_pImpl->m_edges.end(); edgeIt++) {
		if (edgeIt->second->vert2 == vert) return false;
	}
	return true;
}

vector<idType> iAGraphDrawer::getParentsVertices(idType vert) {
	vector<idType> parents;
	for (edgesMap::iterator edgeIt = m_pImpl->m_edges.begin(); edgeIt != m_pImpl->m_edges.end(); edgeIt++) {
		if (edgeIt->second->vert2 == vert)
			parents.push_back(edgeIt->second->vert1);
	}
	return parents;
}

vector<idType> iAGraphDrawer::getChildsVertices(idType vert) {
	vector<idType> childs;
	for (edgesMap::iterator edgeIt = m_pImpl->m_edges.begin(); edgeIt != m_pImpl->m_edges.end(); edgeIt++) {
		if (edgeIt->second->vert1 == vert) childs.push_back(edgeIt->second->vert2);
	}
	return childs;
}

int iAGraphDrawer::getMaximumRank() {
	int maxRank = 0;
	for (verticesMap::iterator vertIt = m_pImpl->m_vertices.begin(); vertIt != m_pImpl->m_vertices.end(); vertIt++) {
		if (vertIt->second->rank > maxRank) maxRank = vertIt->second->rank;
	}
	return maxRank;
}

// int iAGraphDrawer::getMaxNumberOfVertices() {
// 	int maxRank = getMaximumRank();
// 	int maxNumberVertices = 0;
// 	for (int i = 0; i <= maxRank; i++) {
// 		int numberOfVert = getNumberOfVertices(i);
// 		if(numberOfVert > maxNumberVertices)
// 			maxNumberVertices = numberOfVert;
// 	}
// 	return maxNumberVertices;
// }

int iAGraphDrawer::getNumberOfVertices(int rankNum) {
	int verticesInRank = 0;
	for (verticesMap::iterator vertIt = m_pImpl->m_vertices.begin(); vertIt != m_pImpl->m_vertices.end(); vertIt++) {
		if (rankNum == vertIt->second->rank) ++verticesInRank;
	}
	return verticesInRank;
}

const graphVertex* iAGraphDrawer::getVertex(idType vert) {
	return m_pImpl->m_vertices[vert];
}

const graphEdge* iAGraphDrawer::getEdge(idType edge) {
	return m_pImpl->m_edges[edge];
}

/*
========================================
Output
========================================
*/

void iAGraphDrawer::ordering() {

	orderType		tempOrder;
	edgesTableType	edgesTable;

	initialOrder(&m_pImpl->m_order);
	calculateEdgesTable(&edgesTable);

#ifdef DEBUG
	cout<<"Number of crossing before: "<<crossing(&m_order, &edgesTable)<<"\n";
#endif

	tempOrder = m_pImpl->m_order;

#ifdef DEBUG
	cout<<"Number of iterations: "<<m_maxOrderingIterations<<"\n";
#endif

	for (int i = 0; i < m_pImpl->m_maxOrderingIterations; i++) {
		cout<<"Current iteration: "<<i<<"\n";
		
#ifdef DEBUG
		time(&startTime);
		cout<<"Median started... number of crossing: "<<this->crossing(&tempOrder, &edgesTable)<<"\n";
#endif

		wmedian(&tempOrder, i % 2 == 0);
		//wmedian(&tempOrder, true);

#ifdef DEBUG
		time(&endTime);
		cout<<"Median Finished. Elapsed "<<difftime(endTime, startTime)<<" seconds. Number of crossing: "<<this->crossing(&tempOrder, &edgesTable)<<"\n";
		time(&startTime);
		cout<<"Transposing started... number of crossing: "<<this->crossing(&tempOrder, &edgesTable)<<"\n";
#endif

		transpose(&tempOrder, &edgesTable);

#ifdef DEBUG
		time(&endTime);
		cout<<"Transposing finished. Elapsed "<<difftime(endTime, startTime)<<" seconds. Number of crossing: "<<this->crossing(&tempOrder, &edgesTable)<<"\n";
#endif

		if (crossing(&tempOrder, &edgesTable) < crossing(&m_pImpl->m_order, &edgesTable)) {
			m_pImpl->m_order = tempOrder;
		}
	}

#ifdef DEBUG
	cout<<"Number of crossing after: "<<crossing(&m_order, &edgesTable)<<"\n";
#endif
}

void iAGraphDrawer::setPosition() {
	setInitialPositions();

	for(int i = 0; i < m_pImpl->m_maxPositioningIterations; i++) {
		for(int j = 0; j < m_pImpl->m_order.size(); j++) {
			shiftRank(j, i % 2 == 0);
		}
	}
}

void iAGraphDrawer::setInitialPositions() {
	for (size_t i = 0; i < m_pImpl->m_order.size(); i++) {
		for (size_t j = 0; j < m_pImpl->m_order[i].size(); j++) {
			m_pImpl->m_vertices[m_pImpl->m_order[i][j]]->positionX = static_cast<float>(i);
			m_pImpl->m_vertices[m_pImpl->m_order[i][j]]->positionY = static_cast<float>(j);
		}
	}
}

void iAGraphDrawer::shiftRank(int rankNumber, bool forwardDir) {
	vector<idType> destVerts;
	vector<idType> origVerts;
	for(size_t i = 0; i < m_pImpl->m_order[rankNumber].size(); i++) {
		// find adjacent vertices for each vertices in rank
		// and then add they into common array
		vector<idType> verts;
		if(forwardDir)	verts = getParentsVertices(m_pImpl->m_order[rankNumber][i]);
		else			verts = getChildsVertices (m_pImpl->m_order[rankNumber][i]);

		if(verts.size() == 0) continue;

		destVerts.insert(destVerts.end(), verts.begin(), verts.end());
		origVerts.push_back(m_pImpl->m_order[rankNumber][i]);
	}

	if(destVerts.size() == 0) return;

	float destCenterY = 0;
	for(int i = 0; i < destVerts.size(); i++) {
		destCenterY += m_pImpl->m_vertices[destVerts[i]]->positionY;
	}
	destCenterY /= destVerts.size();

	float origCenterY = 0;
	for (int i = 0; i < origVerts.size(); i++) {
		origCenterY += m_pImpl->m_vertices[origVerts[i]]->positionY;
	}
	origCenterY /= origVerts.size();
	
	float shift = destCenterY - origCenterY;
	for(int i = 0; i < m_pImpl->m_order[rankNumber].size(); i++) {
		m_pImpl->m_vertices[m_pImpl->m_order[rankNumber][i]]->positionY += shift;
	}
}

// void iAGraphDrawer::getAverageParentsPosition(idType vertId, float* averagePos) {
// 	getAveragePosition(getParentsVertices(vertId), averagePos);
// }
// 
// void iAGraphDrawer::getAverageChildsPosition(idType vertId, float* averagePos) {
// 	getAveragePosition(getChildsVertices(vertId), averagePos);
// }
// 
// void iAGraphDrawer::getAveragePosition(idType vertId, float* averagePos) {
// 	vector<idType> parents	= getParentsVertices(vertId);
// 	vector<idType> childs	= getChildsVertices(vertId);
// 	vector<idType> vertices (parents.size() + childs.size());			// merge two vectors
// 	vertices.insert(vertices.end(), parents.begin(), parents.end());
// 	vertices.insert(vertices.end(),  childs.begin(),  childs.end());
// 
// 	getAveragePosition(vertices, averagePos);
// }
// 
// void iAGraphDrawer::getAveragePosition(vector<idType> vertices, float* averagePos) {
// 	averagePos[0] = averagePos[1] = 0;
// 
// 	int size = vertices.size();
// 	for(int i = 0; i < size; i++) {
// 		averagePos[0] += m_vertices[vertices[i]]->positionX;
// 		averagePos[1] += m_vertices[vertices[i]]->positionY;
// 	}
// 	averagePos[0] /= size;
// 	averagePos[1] /= size;
// }

/*
===============
Crete initial order
===============
*/
void iAGraphDrawer::initialOrder(orderType* order) {
	if (!order->empty()) order->clear();
	int maxRank = this->getMaximumRank();
	for (int i = 0; i <= maxRank; i++) {
		int numVertices = this->getNumberOfVertices(i);
		vector<idType> vertices (numVertices, -1);
		order->push_back(vertices);
	}

	// find headers vertices
	// then sort them by their rank
	vector<pair<idType, graphVertex*> > headerVerticles;
	for (verticesMap::iterator vertIt = m_pImpl->m_vertices.begin(); vertIt != m_pImpl->m_vertices.end(); vertIt++) {
		if (this->isHeaderVerticle(vertIt->first)) {
			headerVerticles.push_back(make_pair(vertIt->first, m_pImpl->m_vertices[vertIt->first]));
		}
	}
	sort(headerVerticles.begin(), headerVerticles.end(), compare);

	for (size_t i = headerVerticles.size() - 1; i >= 0; i--) {
		this->addVerticesToOrder(headerVerticles[i].first, order);
	}
}

void iAGraphDrawer::calculateEdgesTable(edgesTableType* edgesTable) {
	// initializing
	int maxRank = this->getMaximumRank();
	if (!edgesTable->empty()) edgesTable->clear();
	edgesTable->resize(maxRank);

	// filling
	for (edgesMap::iterator edgeIt = m_pImpl->m_edges.begin(); edgeIt != m_pImpl->m_edges.end(); edgeIt++) {
		int rank = m_pImpl->m_vertices[edgeIt->second->vert1]->rank;
		(*edgesTable)[rank].push_back(edgeIt->first);
	}
}

void iAGraphDrawer::addVerticesToOrder(idType headerVert, orderType* order) {
	graphVertex* v = m_pImpl->m_vertices[headerVert];
	assert(v);
	for (int i = 0; i < (*order)[v->rank].size(); i++) {
		if ((*order)[v->rank][i] == headerVert) {
			return;			// the vertex has been added already
		}
		else if ((*order)[v->rank][i] < 0) {
			// we find a free place in an order then we add a vertex
			(*order)[v->rank][i] = headerVert;

			// add childs of a vertex
			vector<idType> childs = this->getChildsVertices(headerVert);
			for (int j = 0; j < childs.size(); j++) {
				this->addVerticesToOrder(childs[j], order);
			}

			return;			// the vertex has been successfully added
		}
	}
	assert(false);			// FIXME: we did not add a vertex
}

void iAGraphDrawer::setMaxIteration(int maxIter) {
	m_pImpl->m_maxOrderingIterations = maxIter;
}

void iAGraphDrawer::wmedian(orderType* order, bool forwardTraversal) {
	int maxRank = this->getMaximumRank();
	if (forwardTraversal) {
		for (size_t i = 0; i <= maxRank; i++) {
			size_t numVertex = (*order)[i].size();
			vector<pair<idType, float> > median;
			for (size_t j = 0; j < numVertex; j++) {
				float val = medianValue((*order)[i][j], order, forwardTraversal);
				median.push_back(make_pair((*order)[i][j], val));
			}
			sort(median.begin(), median.end(), compareMedians);
			for (size_t j = 0; j < numVertex; j++) {
				(*order)[i][j] = median[j].first;
			}
		}
	}
	else {
		for (int i = maxRank; i >= 0; i--) {
			size_t numVertex = (*order)[i].size();
			vector<pair<idType, float> > median;
			for (int j = 0; j < numVertex; j++) {
				float val = medianValue((*order)[i][j], order, forwardTraversal);
				median.push_back(make_pair((*order)[i][j], val));
			}
			sort(median.begin(), median.end(), compareMedians);
			for (int j = 0; j < numVertex; j++) {
				(*order)[i][j] = median[j].first;
			}
		}
	}
}

float iAGraphDrawer::medianValue(idType vert, orderType* order, bool forwardTraversal) {
	vector<int> p = this->getAdjacentPositions(vert, order, forwardTraversal);
	size_t size = p.size();
	int m = (int)floor((float)size / 2);
	if (size == 0) {
		return -1;
	}
	else if (size % 2 == 1) {
		return (float)p[m];
	}
	else if (size == 2) {
		return (float)(p[0] + p[1]) / 2;
	}
	else {
		int left = p[m - 1] - p[0];
		int right = p[size - 1] - p[m];
		return ((float)p[m - 1] * right + (float)p[m] * left) / (left + right);
	}
}

vector<int> iAGraphDrawer::getAdjacentPositions(idType vert, orderType* order, bool forwardTraversal) {
	vector<idType>	adjacentVerts;
	/*vector<idType> parents = this->getParentsVertices(vert);
	vector<idType> childs = this->getParentsVertices(vert);
	adjacentVerts.insert(adjacentVerts.end(), parents.begin(), parents.end());
	adjacentVerts.insert(adjacentVerts.end(), childs.begin(), childs.end());*/
	if(forwardTraversal)	adjacentVerts = this->getParentsVertices(vert);
	else					adjacentVerts = this->getChildsVertices(vert);


	vector<int>		positions;
	if (adjacentVerts.size() == 0) return positions;

	int rank = m_pImpl->m_vertices[adjacentVerts[0]]->rank;
	for (int i = 0; i < adjacentVerts.size(); i++) {
		assert(rank == m_pImpl->m_vertices[adjacentVerts[i]]->rank);		// FIXME: all adjacent vertices shold located on same rank
		for (int j = 0; j < (*order)[rank].size(); j++) {
			if (adjacentVerts[i] == (*order)[rank][j]) {
				positions.push_back(j);
			}
		}
	}
	assert(positions.size() == adjacentVerts.size());			// FIXME: a rank hasn't all adjacent vertices
	sort(positions.begin(), positions.end());

	return positions;
}

int iAGraphDrawer::crossing(orderType* order, edgesTableType* edgesTable) {
	return this->crossing(order, edgesTable, 0, edgesTable->size());
}

int iAGraphDrawer::crossing(orderType* order, edgesTableType* edgesTable, size_t rank) {
	return this->crossing(order, edgesTable, rank, rank);
}

int iAGraphDrawer::crossing(orderType* order, edgesTableType* edgesTable, size_t startRank, size_t endRank) {
	assert (0 <= startRank && startRank <= endRank && endRank < order->size());
	int		crossNumber = 0;
	idType	v	[2][2];
	size_t	pos	[2][2];

	size_t start	= startRank <= 0 ? 0 : startRank - 1;
	size_t end		= endRank >= edgesTable->size() ? edgesTable->size() : endRank + 1;

	for (size_t i = start; i < end; i++) {
		for (int j = 0; j < (*edgesTable)[i].size(); j++) {
			for (int k = j + 1; k < (*edgesTable)[i].size(); k++) {
				v[0][0] = m_pImpl->m_edges[(*edgesTable)[i][j]]->vert1;
				v[0][1] = m_pImpl->m_edges[(*edgesTable)[i][j]]->vert2;
				v[1][0] = m_pImpl->m_edges[(*edgesTable)[i][k]]->vert1;
				v[1][1] = m_pImpl->m_edges[(*edgesTable)[i][k]]->vert2;

				pos[0][0] = this->vertexPosition(v[0][0], order);
				pos[0][1] = this->vertexPosition(v[0][1], order);
				pos[1][0] = this->vertexPosition(v[1][0], order);
				pos[1][1] = this->vertexPosition(v[1][1], order);

				if		(pos[0][0] < pos[1][0] && pos[0][1] > pos[1][1]) ++crossNumber;
				else if	(pos[0][0] > pos[1][0] && pos[0][1] < pos[1][1]) ++crossNumber;
			}
		}
	}
	return crossNumber;
}

size_t iAGraphDrawer::vertexPosition(idType vert, orderType* order) {
	int rank = m_pImpl->m_vertices[vert]->rank;
	for (size_t i = 0; i < (*order)[rank].size(); i++) {
		if (vert == (*order)[rank][i]) {
			return i;
		}
	}
	assert(false);		// FIXME: a position of vertex is not founded
	return 0;			// wrong, but safe value (otherwise, Undefined Behavior!)
}

void iAGraphDrawer::transpose(orderType* order, edgesTableType* edgesTable) {
	bool improved;

#ifdef DEBUG
	int iteration = 0; cout<<"current iteration of transposing: ";
#endif

	do {

#ifdef DEBUG
		cout<<iteration++<<" ";
#endif

		improved = false;
		for (int i = 0; i < (*order).size(); i++) {
			for (int j = 0; j < (int)(*order)[i].size() - 1; j++) {
				int before = this->crossing(order, edgesTable, i);
				swapVertices(&(*order)[i], j, j + 1);
				int after = this->crossing(order, edgesTable, i);
				if (after < before) {
					improved = true;
				} else {
					swapVertices(&(*order)[i], j, j + 1);
				}
			}
		}
	} while (improved);

#ifdef DEBUG
	cout<<"\n";
#endif

}
