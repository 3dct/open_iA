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
#pragma once

#include "iAGraphDrawer.h"
#include "open_iA_Core_export.h"

#include <vtkMutableDirectedGraph.h>

#include <map>

// forward declarations
class vtkPoints;

class open_iA_Core_API iAVtkGraphDrawer : public iAGraphDrawer {
public:
	void	createLayout(vtkPoints* points, vtkMutableDirectedGraph* graph, int* windowsSize, size_t numRanks);
private:
	void	fillGraph(vtkMutableDirectedGraph* graph);
	void	locatePoints(vtkPoints* points, int* windowsSize, size_t numRanks);

	map<vtkIdType, idType>	m_vertMapFromVtk;
	map<idType, vtkIdType>	m_vertMapToVtk;

// 	map<idType, vtkEdgeType>	m_edgeMapToVtk;
// 	map<vtkEdgeType, idType>	m_edgeMapFromVtk;
};
