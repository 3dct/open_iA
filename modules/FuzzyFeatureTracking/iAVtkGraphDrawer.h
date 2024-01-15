// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGraphDrawer.h>

#include <vtkMutableDirectedGraph.h>

#include <map>

class vtkPoints;

class iAVtkGraphDrawer
{
public:
	void createLayout(vtkPoints* points, vtkMutableDirectedGraph* graph, int* windowsSize, size_t numRanks);

private:
	void fillGraph(vtkMutableDirectedGraph* graph);
	void locatePoints(vtkPoints* points, int* windowsSize, size_t numRanks);

	std::map<vtkIdType, iAGraph::idType> m_vertMapFromVtk;
	std::map<iAGraph::idType, vtkIdType> m_vertMapToVtk;
	iAGraphDrawer m_graphDrawer;
	iAGraph m_graph;
};
