// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAGraph.h"
#include "iAGraphStat.h"

#include <cstddef>
#include <vector>

class iAGraphDrawer
{
public:
	iAGraphDrawer();
	~iAGraphDrawer();
	void setNumberOfIterations(int n);
	void setGraph(iAGraph* g);
	void start();

private:
	typedef std::vector<std::vector<iAGraph::idType>> OrderType;

	void initialOrder(OrderType& order);
	void addVerticesToOrder(iAGraph::idType headerVert, OrderType& order);
	void wmedian(OrderType& order, bool forwardTraversal);
	float medianValue(iAGraph::idType vert, OrderType& order, bool forwardTraversal);
	std::vector<size_t> getAdjacentPositions(iAGraph::idType vert, OrderType& order, bool forwardTraversal);
	void transpose(OrderType& order, bool forwardTraversal);
	int numberOfCrossing(OrderType& order);
	int numberOfCrossing(OrderType& order, size_t rank, size_t pos1, size_t pos2, bool forwardTraversal);

	int m_maxIteration;
	OrderType m_order;
	iAGraph* m_graph;
	iAGraphStat m_graphStat;
};
