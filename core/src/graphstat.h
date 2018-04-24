/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

#include "graph.h"

#include <vector>
#include <unordered_map>

class GraphStat
{
public:
	typedef std::vector<Graph::idType>	VerticesIDs;

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
	std::unordered_map<Graph::idType, VerticesIDs>	m_parentVertices;
	std::unordered_map<Graph::idType, VerticesIDs>	m_childVertices;
	std::unordered_map<Graph::idType, bool>			m_isHeaderVertex;
	std::vector<int>								m_numVertices;
};

