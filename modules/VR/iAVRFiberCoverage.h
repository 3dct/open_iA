/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include <vtkIdTypeArray.h>
#include <vtkTable.h>

#include <unordered_map>

#include "iACsvIO.h"
#include "iAVRVolume.h"
#include "iAVROctree.h"

/*
* This class maps the fibers Object ID in the csv file to the polyObject ID of the
* rendered points and calculates the coverage of fiber inside octree regions
*/
class iAVRFiberCoverage
{
public:
	iAVRFiberCoverage(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees, iAVRVolume* volume);
	void mapAllPointiDs();
	void mapAllPointiDsAndCalculateFiberCoverage();
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* getFiberCoverage();
	vtkIdType getObjectiD(vtkIdType polyPoint);
	std::unordered_map<vtkIdType, vtkIdType> getPointIDToCsvIndexMapper();
	std::unordered_multimap<vtkIdType, vtkIdType> getCsvIndexToPointIDMapper();

private:
	vtkSmartPointer<vtkTable> m_objectTable;
	iACsvIO m_io;
	std::vector<iAVROctree*>* m_octrees;
	iAVRVolume* m_volume;
	// Maps poly point IDs to Object IDs in csv file
	std::unordered_map<vtkIdType, vtkIdType> m_pointIDToCsvIndex;
	// Maps Object IDs in csv file to their poly point IDs (1 fiber has 2 points)
	std::unordered_multimap<vtkIdType, vtkIdType> m_csvIndexToPointID;
	//Stores for the [octree level] in an [octree region] a map of its fiberIDs with their coverage
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* m_fiberCoverage;

	vtkSmartPointer<vtkPoints> getOctreeFiberCoverage(double startPoint[3], double endPoint[3], int octreeLevel, int fiber, double fiberLength);
	bool checkIntersectionWithBox(double startPoint[3], double endPoint[3], std::vector<std::vector<iAVec3d>>* planePoints, double bounds[6], double intersection[3]);
	double calculateFiberCoverage(double startPoint[3], double endPoint[3], double fiberLength);
	bool checkEqualArrays(float pos1[3], float pos2[3]);
	bool checkEqualArrays(double pos1[3], double pos2[3]);
};