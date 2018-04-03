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

#include "graph.h"

Graph::Graph()
	: m_curInd{0}
{ }

Graph::idType Graph::addVertex(Vertex vert) {
	idType id = getNewID();
	m_vetices[id] = vert;
	return id;
}

Graph::idType Graph::addEdge(idType v1, idType v2) {
	idType id = getNewID();
	m_edges[id] = Edge(v1, v2);
	return id;
}

Graph::idType Graph::getNewID() {
	return m_curInd++;
}

Graph::VerticesMap* Graph::getVertices()
{
	return &m_vetices;
}

Graph::EdgesMap* Graph::getEdges()
{
	return &m_edges;
}