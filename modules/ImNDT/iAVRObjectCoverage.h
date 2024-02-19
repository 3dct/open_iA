// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iACsvConfig.h>

#include <iAVec3.h>

#include <vtkSmartPointer.h>

#include <unordered_map>

class iAVRObjectModel;
class iAVROctree;

class vtkPoints;
class vtkTable;

//! Calculates the coverage of objects inside octree regions.
class iAVRObjectCoverage
{
public:
	iAVRObjectCoverage(vtkTable* objectTable, iAColMapP colMapping, iACsvConfig csvConfig, std::vector<iAVROctree*> const & octrees, iAVRObjectModel* volume);
	void calculateObjectCoverage();
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* getObjectCoverage();
	vtkIdType getObjectiD(vtkIdType polyPoint);

private:
	vtkSmartPointer<vtkTable> m_objectTable;
	iAColMapP m_mapping;
	iACsvConfig m_csvConfig;
	std::vector<iAVROctree*> const & m_octrees;
	iAVRObjectModel* m_volume;
	//Stores for the [octree level] in an [octree region] a map of its objectIDs with their coverage
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>> m_objectCoverage;

	void initialize();
	void calculateLineCoverage();
	void calculateCurvedLineCoverage();
	void calculateEllipsoidCoverage();
	vtkSmartPointer<vtkPoints> getOctreeFiberCoverage(double startPoint[3], double endPoint[3], vtkIdType octreeLevel, vtkIdType fiber, double fiberLength);
	bool checkIntersectionWithBox(double startPoint[3], double endPoint[3], double bounds[6], double intersection[3]);
	double calculateLineCoverageRatio(double startPoint[3], double endPoint[3], double lineLength);
	void storeObjectCoverage(vtkIdType octreeLevel, vtkIdType region, vtkIdType fiber, double coverage);
	bool checkEqualArrays(float pos1[3], float pos2[3]);
	bool checkEqualArrays(double pos1[3], double pos2[3]);
	void createPlanePoint(int plane, double bounds[6], iAVec3d* planeOrigin, iAVec3d* planeP1, iAVec3d* planeP2);
	void printObjectCoverage();
};
