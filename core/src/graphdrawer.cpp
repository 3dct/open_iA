/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "graphdrawer.h"

#include <algorithm>
#include <assert.h>
#include <iostream>

#include <cmath>

bool compare(pair<Graph::idType, Graph::Vertex> v1, pair<Graph::idType, Graph::Vertex> v2) {
	return (v1.second.rank < v2.second.rank);
}

bool compareMedians(pair<Graph::idType, float> v1, pair<Graph::idType, float> v2) {
	return (v1.second < v2.second);
}

void swapVertices(vector<Graph::idType>& rank, int index1, int index2) {
	assert(&rank);
	assert((size_t)index1 < rank.size());
	assert((size_t)index2 < rank.size());

	Graph::idType val = rank[index1];
	rank[index1] = rank[index2];
	rank[index2] = val;
}

GraphDrawer::GraphDrawer()
	: m_maxIteration{4}
{ }

GraphDrawer::~GraphDrawer()
{ }

void GraphDrawer::setNumberOfIterations(int n)
{
	m_maxIteration = n;
}

void GraphDrawer::setGraph(Graph* g)
{
	m_graph = g;
	m_graphStat.setGraph(m_graph);
	m_graphStat.update();
}

void GraphDrawer::initialOrder(OrderType& order)
{
	if (!order.empty()) order.clear();
	for (int i = 0; i <= m_graphStat.getMaxRank(); i++) {
		vector<Graph::idType> vertices(m_graphStat.getNumVerticesInRank(i), -1);
		order.push_back(vertices);
	}

	// find headers vertices
	// then sort them by their rank
	vector<pair<Graph::idType, Graph::Vertex>> headerVerticles;
	for (Graph::VerticesMap::const_iterator vertIt = m_graph->getVertices()->begin(); vertIt != m_graph->getVertices()->end(); vertIt++) {
		if (m_graphStat.isHeaderVertix(vertIt->first)) {
			headerVerticles.push_back(make_pair(vertIt->first, m_graph->getVertices()->at(vertIt->first)));
		}
	}
	sort(headerVerticles.begin(), headerVerticles.end(), compare);

	for (int i = headerVerticles.size() - 1; i >= 0; i--) {
		this->addVerticesToOrder(headerVerticles[i].first, order);
	}
}

void GraphDrawer::addVerticesToOrder(Graph::idType headerVert, OrderType& order)
{
	Graph::Vertex v = m_graph->getVertices()->at(headerVert);
	for (size_t i = 0; i < order[v.rank].size(); i++) {
		if (order[v.rank][i] == headerVert) {
			return;			// the vertex has been added already
		}
		else if (order[v.rank][i] < 0) {
			// we find a free place in an order then we add a vertex
			order[v.rank][i] = headerVert;

			// add childs of a vertex
			GraphStat::VerticesIDs childs = m_graphStat.getChildVertices(headerVert);
			for (size_t j = 0; j < childs.size(); j++) {
				this->addVerticesToOrder(childs[j], order);
			}

			return;			// the vertex has been successfully added
		}
	}
	assert(false);			// FIXME: we did not add a vertex
}

void GraphDrawer::start()
{
	OrderType		tempOrder;

	initialOrder(m_order);
	tempOrder = m_order;
	for (int i = 0; i < m_maxIteration; i++) {
		bool forward = i % 2;
		//wmedian(&tempOrder, i % 2 == 0);
		wmedian(tempOrder, forward);
		transpose(tempOrder, forward);

		if (numberOfCrossing(tempOrder) < numberOfCrossing(m_order)) {
			m_order = tempOrder;
		}
	}

	std::cout << "crossings " << numberOfCrossing(m_order);

	for (size_t i = 0; i < m_order.size(); i++) {
		for (size_t j = 0; j < m_order[i].size(); j++) {
			float displacement = (m_order[i].size() - 1) / 2;
			m_graph->getVertices()->at(m_order[i][j]).posX = (float)i;
			m_graph->getVertices()->at(m_order[i][j]).posY = (float)j - displacement;
		}
	}
}

void GraphDrawer::wmedian(OrderType& order, bool forwardTraversal)
{
	int maxRank = m_graphStat.getMaxRank();
	if (forwardTraversal) {
		for (int i = 0; i <= maxRank; i++) {
			int numVertex = order[i].size();
			vector<pair<Graph::idType, float>> median;
			for (int j = 0; j < numVertex; j++) {
				float val = medianValue(order[i][j], order, forwardTraversal);
				median.push_back(make_pair(order[i][j], val));
			}
			sort(median.begin(), median.end(), compareMedians);
			for (int j = 0; j < numVertex; j++) {
				order[i][j] = median[j].first;
			}
		}
	}
	else {
		for (int i = maxRank; i >= 0; i--) {
			int numVertex = order[i].size();
			vector<pair<Graph::idType, float>> median;
			for (int j = 0; j < numVertex; j++) {
				float val = medianValue(order[i][j], order, forwardTraversal);
				median.push_back(make_pair(order[i][j], val));
			}
			sort(median.begin(), median.end(), compareMedians);
			for (int j = 0; j < numVertex; j++) {
				order[i][j] = median[j].first;
			}
		}
	}
}

float GraphDrawer::medianValue(Graph::idType vert, OrderType& order, bool forwardTraversal)
{
	vector<int> p = getAdjacentPositions(vert, order, forwardTraversal);
	int size = p.size();
	int m = (int)std::floor((float)size / 2);
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

std::vector<int> GraphDrawer::getAdjacentPositions(Graph::idType vert, OrderType& order, bool forwardTraversal)
{
	vector<Graph::idType> adjacentVerts;
	if(forwardTraversal) {
		adjacentVerts = m_graphStat.getParentVertices(vert);
	}
	else {
		adjacentVerts = m_graphStat.getChildVertices(vert);
	}
	vector<int>	positions;
	if (adjacentVerts.size() == 0) return positions;

	int rank = m_graph->getVertices()->at(adjacentVerts[0]).rank;
	for (size_t i = 0; i < adjacentVerts.size(); i++) {
		assert(rank == m_graph->getVertices()->at(adjacentVerts[i]).rank);		// FIXME: all adjacent vertices should be located at the same rank
		for (size_t j = 0; j < order[rank].size(); j++) {
			if (adjacentVerts[i] == order[rank][j]) {
				positions.push_back(j);
			}
		}
	}
	assert(positions.size() == adjacentVerts.size());			// FIXME: a rank hasn't all adjacent vertices
	sort(positions.begin(), positions.end());

	return positions;
}

void GraphDrawer::transpose(OrderType& order, bool forwardTraversal)
{
	bool improved;
	do {
		improved = false;
		for (size_t i = 0; i+1 < order.size(); i++) {
			for (size_t j = 0; j+1 < order[i].size(); j++) {
				int before = numberOfCrossing(order, i, j, j+1, forwardTraversal);
				swapVertices(order[i], j, j + 1);
				int after = numberOfCrossing(order, i, j, j+1, forwardTraversal);
				if (after < before) {
					improved = true;
				} else {
					swapVertices(order[i], j, j + 1);
				}
			}
		}
	} while (improved);
}

int GraphDrawer::numberOfCrossing(OrderType& order) {
	int crossings = 0;
	for(size_t i = 0; i+1 < order.size(); i++)
		for(size_t j = 0; j+1 < order[i].size(); j++)
			crossings = crossings + numberOfCrossing(order, i, j, j+1, true);
	return crossings;
}

int GraphDrawer::numberOfCrossing(OrderType& order, int rank, int pos1, int pos2, bool forwardTraversal) {
	/*GraphStat::VerticesIDs topChilds = m_graphStat.getChildVertices(order[rank][pos1]);
	GraphStat::VerticesIDs bottomChilds = m_graphStat.getChildVertices(order[rank][pos2]);*/
	GraphStat::VerticesIDs topChilds, bottomChilds;
	if(forwardTraversal) {
		topChilds = m_graphStat.getChildVertices(order[rank][pos1]);
		bottomChilds = m_graphStat.getChildVertices(order[rank][pos2]);
	} else {
		topChilds = m_graphStat.getParentVertices(order[rank][pos1]);
		bottomChilds = m_graphStat.getParentVertices(order[rank][pos2]);
	}

	std::vector<int> topPos, bottomPos;
	for(auto vertId : topChilds)
		for(size_t j = 0; j < order[rank+1].size(); j++)
			if(order[rank+1][j] == vertId) {
				topPos.push_back(j);
				break;
			}
	for(auto vertId : bottomChilds)
		for(size_t j = 0; j < order[rank+1].size(); j++)
			if(order[rank+1][j] == vertId) {
				bottomPos.push_back(j);
				break;
			}

	int crossings = 0;
	for(size_t i = 0; i < topPos.size(); i++)
		for(size_t j = 0; j < bottomPos.size(); j++)
			if(topPos[i] > bottomPos[j]) crossings++;
	return crossings;
}
