/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAVtkGraphDrawer.h"

#include <vtkSmartPointer.h>
#include <vtkVertexListIterator.h>
#include <vtkEdgeListIterator.h>
#include <vtkDataSetAttributes.h>
#include <vtkPoints.h>

#include <cassert>

void iAVtkGraphDrawer::createLayout(vtkPoints* points, vtkMutableDirectedGraph* graph, int* windowsSize, size_t numRanks) {
	this->fillGraph(graph);
	this->ordering();
	this->setPosition();
	this->locatePoints(points, windowsSize, numRanks);
}

void iAVtkGraphDrawer::fillGraph(vtkMutableDirectedGraph* graph) {
	vtkSmartPointer<vtkVertexListIterator> vertIt = vtkSmartPointer<vtkVertexListIterator>::New();
	graph->GetVertices(vertIt);
	while(vertIt->HasNext()) {
		vtkIdType idVtk = vertIt->Next();
		int rank = graph->GetVertexData()->GetAbstractArray("Layer")->GetVariantValue(idVtk).ToInt();
		idType id = this->addVertex(graphVertex(0, 0, rank));
		m_vertMapToVtk[id] = idVtk;
		m_vertMapFromVtk[idVtk] = id;
	}

	vtkSmartPointer<vtkEdgeListIterator> edgeIt = vtkSmartPointer<vtkEdgeListIterator>::New();
	graph->GetEdges(edgeIt);
	while(edgeIt->HasNext()) {
		vtkEdgeType idVtk = edgeIt->Next();
		int rankSource = graph->GetVertexData()->GetAbstractArray("Layer")->GetVariantValue(idVtk.Source).ToInt();
		int rankTarget = graph->GetVertexData()->GetAbstractArray("Layer")->GetVariantValue(idVtk.Target).ToInt();
		assert (rankSource != rankTarget);		// FIMXE: we can't have a edges are connected two vertex on a same rank
		idType id;
		if (rankSource < rankTarget) {
			id = this->addEdge(m_vertMapFromVtk[idVtk.Source], m_vertMapFromVtk[idVtk.Target]);
		} else {
			id = this->addEdge(m_vertMapFromVtk[idVtk.Target], m_vertMapFromVtk[idVtk.Source]);
		}
// 		m_edgeMapToVtk[id] = idVtk;
// 		m_edgeMapFromVtk[idVtk] = id;
	}
}

void iAVtkGraphDrawer::locatePoints(vtkPoints* points, int* windowsSize, size_t numRanks) {
	map<vtkIdType, idType>::iterator it;
	float maxPosY, minPosY;
	maxPosY = minPosY = this->getVertex(m_vertMapFromVtk.at(0))->positionY;
	for (it = m_vertMapFromVtk.begin(); it != m_vertMapFromVtk.end(); it++) {
		if (maxPosY < (this->getVertex(it->second))->positionY) {
			maxPosY = (this->getVertex(it->second))->positionY;
		}
		if (minPosY > (this->getVertex(it->second))->positionY) {
			minPosY = (this->getVertex(it->second))->positionY;
		}
	}

	float divisions[2];
	divisions[0] = (float)windowsSize[0] / numRanks;
	divisions[1] = (float)windowsSize[1] / (maxPosY - minPosY + 1);

	for (it = m_vertMapFromVtk.begin(); it != m_vertMapFromVtk.end(); it++) {
		points->InsertNextPoint(divisions[0] / 2 + divisions[0] * this->getVertex(it->second)->positionX, divisions[1] / 2 + divisions[1] * (this->getVertex(it->second)->positionY - minPosY), 0);
	}
}
