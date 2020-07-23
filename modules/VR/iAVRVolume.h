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

#include "iAVRCubicRepresentation.h"
#include "iACsvIO.h"

#include <vtkTable.h>

#include <QStandardItem>

#include <unordered_map>


class iA3DCylinderObjectVis;

//! Class which represents the rendered volume
class iAVRVolume: public iAVRCubicRepresentation
{
public:
	iAVRVolume(vtkRenderer* ren, vtkTable* objectTable, iACsvIO io);
	void resetVolume();
	void showVolume();
	void hideVolume();
	void createCubeModel() override;
	void showRegionLinks();
	void hideRegionLinks();
	vtkSmartPointer<vtkActor> getVolumeActor();
	void setMappers(std::unordered_map<vtkIdType, vtkIdType> pointIDToCsvIndex, std::unordered_multimap<vtkIdType, vtkIdType> csvIndexToPointID);
	vtkSmartPointer<vtkPolyData> getVolumeData();
	void renderSelection(std::vector<size_t> const& sortedSelInds, int classID, QColor const& classColor, QStandardItem* activeClassItem);
	void createNewVolume(std::vector<size_t> fiberIDs);
	void moveFibersByMaxCoverage(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset);
	void moveFibersbyAllCoveredRegions(double offset);
	void createRegionLinks(std::vector<std::vector<std::vector<double>>>* similarityMetric, double maxFibersInRegions);

private:
	vtkSmartPointer<vtkActor> m_volumeActor;
	vtkSmartPointer<vtkActor> m_RegionLinksActor;
	vtkSmartPointer<vtkActor> m_RegionNodesActor;
	iA3DCylinderObjectVis* m_cylinderVis;
	vtkSmartPointer<vtkTable> m_objectTable;
	vtkSmartPointer<vtkPolyData> m_linePolyData;
	std::unordered_map<vtkIdType, vtkIdType> m_pointIDToCsvIndex;
	std::unordered_multimap<vtkIdType, vtkIdType> m_csvIndexToPointID;
	iACsvIO m_io;
	bool m_volumeVisible;
	bool m_regionLinksVisible;
	void createRegionNodes(double maxFibersInRegions);
};