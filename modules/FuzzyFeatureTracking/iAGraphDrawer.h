/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#pragma once

#include "iAGraph.h"
#include "iAGraphStat.h"

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
	int m_ranks;
};

