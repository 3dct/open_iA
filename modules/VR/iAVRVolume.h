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

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkTable.h>
#include <vtkDataSet.h>
#include <vtkPolyData.h>

#include <QColor>
#include <QStandardItem>

#include <unordered_map>

#include "iACsvIO.h"
#include "iAVROctree.h"

class iA3DCylinderObjectVis;

//! 
class iAVRVolume
{
public:
	iAVRVolume(vtkRenderer* ren, vtkTable* objectTable, iACsvIO io);
	void setOctrees(iAVROctree* octree);
	void setMappers(std::unordered_map<vtkIdType, vtkIdType> pointIDToCsvIndex, std::unordered_multimap<vtkIdType, vtkIdType> csvIndexToPointID);
	void show();
	void hide();
	vtkSmartPointer<vtkPolyData> getPolyData();
	vtkSmartPointer<vtkActor> getActor();
	void renderSelection(std::vector<size_t> const& sortedSelInds, int classID, QColor const& classColor, QStandardItem* activeClassItem);
	void createNewVolume(std::vector<size_t> fiberIDs);
	void moveRegions(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset);
	void moveFibersbyAllCoveredRegions(double offset);

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkActor> m_actor;
	iA3DCylinderObjectVis* m_cylinderVis;
	vtkSmartPointer<vtkTable> m_objectTable;
	iAVROctree* m_octree;
	std::unordered_map<vtkIdType, vtkIdType> m_pointIDToCsvIndex;
	std::unordered_multimap<vtkIdType, vtkIdType> m_csvIndexToPointID;
	iACsvIO m_io;
	bool m_visible;
};