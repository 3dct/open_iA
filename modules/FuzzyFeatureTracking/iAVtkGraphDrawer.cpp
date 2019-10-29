/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVtkGraphDrawer.h"

#include <vtkDataSetAttributes.h>
#include <vtkEdgeListIterator.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkVertexListIterator.h>

#include <cassert>

void iAVtkGraphDrawer::createLayout(vtkPoints* points, vtkMutableDirectedGraph* graph, int* windowsSize, size_t numRanks)
{
	fillGraph(graph);
	m_graphDrawer.setNumberOfIterations(40);
	m_graphDrawer.start();
	locatePoints(points, windowsSize, numRanks);
}

void iAVtkGraphDrawer::fillGraph(vtkMutableDirectedGraph* graph)
{
	vtkSmartPointer<vtkVertexListIterator> vertIt = vtkSmartPointer<vtkVertexListIterator>::New();
	graph->GetVertices(vertIt);
	while(vertIt->HasNext())
	{
		vtkIdType idVtk = vertIt->Next();
		int rank = graph->GetVertexData()->GetAbstractArray("Layer")->GetVariantValue(idVtk).ToInt();
		iAGraph::idType id = m_graph.addVertex(iAGraph::Vertex(rank, 0, 0));
		m_vertMapToVtk[id] = idVtk;
		m_vertMapFromVtk[idVtk] = id;
	}

	vtkSmartPointer<vtkEdgeListIterator> edgeIt = vtkSmartPointer<vtkEdgeListIterator>::New();
	graph->GetEdges(edgeIt);
	while(edgeIt->HasNext())
	{
		vtkEdgeType idVtk = edgeIt->Next();
		int rankSource = graph->GetVertexData()->GetAbstractArray("Layer")->GetVariantValue(idVtk.Source).ToInt();
		int rankTarget = graph->GetVertexData()->GetAbstractArray("Layer")->GetVariantValue(idVtk.Target).ToInt();
		assert (rankSource != rankTarget);		// FIMXE: we can't have a edges are connected two vertex on a same rank
		iAGraph::idType id;
		if (rankSource < rankTarget)
		{
			id = m_graph.addEdge(m_vertMapFromVtk[idVtk.Source], m_vertMapFromVtk[idVtk.Target]);
		}
		else
		{
			id = m_graph.addEdge(m_vertMapFromVtk[idVtk.Target], m_vertMapFromVtk[idVtk.Source]);
		}
// 		m_edgeMapToVtk[id] = idVtk;
// 		m_edgeMapFromVtk[idVtk] = id;
	}
	m_graphDrawer.setGraph(&m_graph);
}

void iAVtkGraphDrawer::locatePoints(vtkPoints* points, int* windowsSize, size_t numRanks)
{
	std::map<vtkIdType, iAGraph::idType>::iterator it;
	float maxPosY, minPosY;
	//maxPosY = minPosY = this->getVertex(m_vertMapFromVtk.at(0))->positionY;
	maxPosY = minPosY = m_graph.getVertices()->at(m_vertMapFromVtk.at(0)).posX;
	for (it = m_vertMapFromVtk.begin(); it != m_vertMapFromVtk.end(); it++)
	{
		if (maxPosY < m_graph.getVertices()->at(it->second).posX)
		{
			maxPosY = m_graph.getVertices()->at(it->second).posY;
		}
		if (minPosY > m_graph.getVertices()->at(it->second).posY)
		{
			minPosY = m_graph.getVertices()->at(it->second).posY;
		}
	}

	float divisions[2];
	divisions[0] = (float)windowsSize[0] / numRanks;
	divisions[1] = (float)windowsSize[1] / (maxPosY - minPosY + 1);

	for (it = m_vertMapFromVtk.begin(); it != m_vertMapFromVtk.end(); it++)
	{
		points->InsertNextPoint(divisions[0] / 2 + divisions[0] * m_graph.getVertices()->at(it->second).posX,
			divisions[1] / 2 + divisions[1] * (m_graph.getVertices()->at(it->second).posY * 0.3 - minPosY), 0);
	}
}
