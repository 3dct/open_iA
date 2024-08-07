// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGraphDrawer.h"

#include <iALog.h>

#include <QString>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

bool compare(std::pair<iAGraph::idType, iAGraph::Vertex> v1, std::pair<iAGraph::idType, iAGraph::Vertex> v2)
{
	return (v1.second.rank < v2.second.rank);
}

bool compareMedians(std::pair<iAGraph::idType, float> v1, std::pair<iAGraph::idType, float> v2)
{
	return (v1.second < v2.second);
}

void swapVertices(std::vector<iAGraph::idType>& rank, size_t index1, size_t index2)
{
	assert(&rank);
	assert(index1 < rank.size());
	assert(index2 < rank.size());

	iAGraph::idType val = rank[index1];
	rank[index1] = rank[index2];
	rank[index2] = val;
}

iAGraphDrawer::iAGraphDrawer():
	m_maxIteration{4},
	m_graph(nullptr)
{ }

iAGraphDrawer::~iAGraphDrawer()
{ }

void iAGraphDrawer::setNumberOfIterations(int n)
{
	m_maxIteration = n;
}

void iAGraphDrawer::setGraph(iAGraph* g)
{
	m_graph = g;
	m_graphStat.setGraph(m_graph);
	m_graphStat.update();
}

void iAGraphDrawer::initialOrder(OrderType& order)
{
	if (!order.empty())
	{
		order.clear();
	}
	for (int i = 0; i <= m_graphStat.getMaxRank(); i++)
	{
		std::vector<iAGraph::idType> vertices(m_graphStat.getNumVerticesInRank(i), -1);
		order.push_back(vertices);
	}

	// find headers vertices
	// then sort them by their rank
	std::vector<std::pair<iAGraph::idType, iAGraph::Vertex>> headerVertices;
	for (auto vertIt = m_graph->getVertices()->begin(); vertIt != m_graph->getVertices()->end(); vertIt++)
	{
		if (m_graphStat.isHeaderVertix(vertIt->first))
		{
			headerVertices.push_back(std::make_pair(vertIt->first, m_graph->getVertices()->at(vertIt->first)));
		}
	}
	sort(headerVertices.begin(), headerVertices.end(), compare);
	assert(headerVertices.size() <= static_cast<size_t>(std::numeric_limits<int>::max()));
	for (int i = static_cast<int>(headerVertices.size() - 1); i >= 0; i--)
	{
		this->addVerticesToOrder(headerVertices[i].first, order);
	}
}

void iAGraphDrawer::addVerticesToOrder(iAGraph::idType headerVert, OrderType& order)
{
	iAGraph::Vertex v = m_graph->getVertices()->at(headerVert);
	for (size_t i = 0; i < order[v.rank].size(); i++)
	{
		if (order[v.rank][i] == headerVert)
		{
			return;			// the vertex has been added already
		}
		else if (order[v.rank][i] < 0)
		{
			// we find a free place in an order then we add a vertex
			order[v.rank][i] = headerVert;

			// add childs of a vertex
			iAGraphStat::VerticesIDs childs = m_graphStat.getChildVertices(headerVert);
			for (size_t j = 0; j < childs.size(); j++)
			{
				this->addVerticesToOrder(childs[j], order);
			}

			return;			// the vertex has been successfully added
		}
	}
	assert(false);			// FIXME: we did not add a vertex
}

void iAGraphDrawer::start()
{
	OrderType		tempOrder;

	initialOrder(m_order);
	tempOrder = m_order;
	for (int i = 0; i < m_maxIteration; i++)
	{
		bool forward = i % 2;
		//wmedian(&tempOrder, i % 2 == 0);
		wmedian(tempOrder, forward);
		transpose(tempOrder, forward);

		if (numberOfCrossing(tempOrder) < numberOfCrossing(m_order))
		{
			m_order = tempOrder;
		}
	}

	LOG(lvlDebug, QString("Crossings: %1").arg(numberOfCrossing(m_order)));

	for (size_t i = 0; i < m_order.size(); i++)
	{
		for (size_t j = 0; j < m_order[i].size(); j++)
		{
			float displacement = static_cast<float>((m_order[i].size() - 1) / 2);
			m_graph->getVertices()->at(m_order[i][j]).posX = static_cast<float>(i);
			m_graph->getVertices()->at(m_order[i][j]).posY = static_cast<float>(j) - displacement;
		}
	}
}

void iAGraphDrawer::wmedian(OrderType& order, bool forwardTraversal)
{
	int maxRank = m_graphStat.getMaxRank();
	if (forwardTraversal)
	{
		for (int i = 0; i <= maxRank; i++)
		{
			size_t numVertex = order[i].size();
			std::vector<std::pair<iAGraph::idType, float>> median;
			for (size_t j = 0; j < numVertex; j++)
			{
				float val = medianValue(order[i][j], order, forwardTraversal);
				median.push_back(std::make_pair(order[i][j], val));
			}
			sort(median.begin(), median.end(), compareMedians);
			for (size_t j = 0; j < numVertex; j++)
			{
				order[i][j] = median[j].first;
			}
		}
	}
	else
	{
		for (int i = maxRank; i >= 0; i--)
		{
			assert(order[i].size() <= std::numeric_limits<int>::max());
			int numVertex = static_cast<int>(order[i].size());
			std::vector<std::pair<iAGraph::idType, float>> median;
			for (int j = 0; j < numVertex; j++)
			{
				float val = medianValue(order[i][j], order, forwardTraversal);
				median.push_back(std::make_pair(order[i][j], val));
			}
			sort(median.begin(), median.end(), compareMedians);
			for (int j = 0; j < numVertex; j++)
			{
				order[i][j] = median[j].first;
			}
		}
	}
}

float iAGraphDrawer::medianValue(iAGraph::idType vert, OrderType& order, bool forwardTraversal)
{
	auto p = getAdjacentPositions(vert, order, forwardTraversal);
	size_t size = p.size();
	int m = static_cast<int>(std::floor(static_cast<float>(size) / 2));
	if (size == 0)
	{
		return -1;
	}
	else if (size % 2 == 1)
	{
		return static_cast<float>(p[m]);
	}
	else if (size == 2)
	{
		return static_cast<float>(p[0] + p[1]) / 2;
	}
	else
	{
		size_t left = p[m - 1] - p[0];
		size_t right = p[size - 1] - p[m];
		return (static_cast<float>(p[m - 1]) * right + static_cast<float>(p[m]) * left) / (left + right);
	}
}

std::vector<size_t> iAGraphDrawer::getAdjacentPositions(iAGraph::idType vert, OrderType& order, bool forwardTraversal)
{
	std::vector<iAGraph::idType> adjacentVerts;
	if(forwardTraversal)
	{
		adjacentVerts = m_graphStat.getParentVertices(vert);
	}
	else
	{
		adjacentVerts = m_graphStat.getChildVertices(vert);
	}
	std::vector<size_t> positions;
	if (adjacentVerts.size() == 0)
	{
		return positions;
	}

	int rank = m_graph->getVertices()->at(adjacentVerts[0]).rank;
	for (size_t i = 0; i < adjacentVerts.size(); i++)
	{
		assert(rank == m_graph->getVertices()->at(adjacentVerts[i]).rank);		// FIXME: all adjacent vertices should be located at the same rank
		for (size_t j = 0; j < order[rank].size(); j++)
		{
			if (adjacentVerts[i] == order[rank][j])
			{
				positions.push_back(j);
			}
		}
	}
	assert(positions.size() == adjacentVerts.size());			// FIXME: a rank hasn't all adjacent vertices
	sort(positions.begin(), positions.end());

	return positions;
}

void iAGraphDrawer::transpose(OrderType& order, bool forwardTraversal)
{
	bool improved;
	do
	{
		improved = false;
		for (size_t i = 0; i+1 < order.size(); i++)
		{
			for (size_t j = 0; j+1 < order[i].size(); j++)
			{
				int before = numberOfCrossing(order, i, j, j+1, forwardTraversal);
				swapVertices(order[i], j, j + 1);
				int after = numberOfCrossing(order, i, j, j+1, forwardTraversal);
				if (after < before)
				{
					improved = true;
				}
				else
				{
					swapVertices(order[i], j, j + 1);
				}
			}
		}
	} while (improved);
}

int iAGraphDrawer::numberOfCrossing(OrderType& order)
{
	int crossings = 0;
	for (size_t i = 0; i + 1 < order.size(); i++)
	{
		for (size_t j = 0; j + 1 < order[i].size(); j++)
		{
			crossings = crossings + numberOfCrossing(order, i, j, j + 1, true);
		}
	}
	return crossings;
}

int iAGraphDrawer::numberOfCrossing(OrderType& order, size_t rank, size_t pos1, size_t pos2, bool forwardTraversal)
{
	/*iAGraphStat::VerticesIDs topChilds = m_graphStat.getChildVertices(order[rank][pos1]);
	iAGraphStat::VerticesIDs bottomChilds = m_graphStat.getChildVertices(order[rank][pos2]);*/
	iAGraphStat::VerticesIDs topChilds, bottomChilds;
	if(forwardTraversal)
	{
		topChilds = m_graphStat.getChildVertices(order[rank][pos1]);
		bottomChilds = m_graphStat.getChildVertices(order[rank][pos2]);
	}
	else
	{
		topChilds = m_graphStat.getParentVertices(order[rank][pos1]);
		bottomChilds = m_graphStat.getParentVertices(order[rank][pos2]);
	}

	std::vector<size_t> topPos, bottomPos;
	for (auto vertId : topChilds)
	{
		for (size_t j = 0; j < order[rank + 1].size(); j++)
		{
			if (order[rank + 1][j] == vertId)
			{
				topPos.push_back(j);
				break;
			}
		}
	}
	for (auto vertId : bottomChilds)
	{
		for (size_t j = 0; j < order[rank + 1].size(); j++)
		{
			if (order[rank + 1][j] == vertId)
			{
				bottomPos.push_back(j);
				break;
			}
		}
	}

	int crossings = 0;
	for (size_t i = 0; i < topPos.size(); i++)
	{
		for (size_t j = 0; j < bottomPos.size(); j++)
		{
			if (topPos[i] > bottomPos[j])
			{
				crossings++;
			}
		}
	}
	return crossings;
}
