// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkDataSet.h>
#include <vtkOctreePointLocator.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>

#include <iAVec3.h>

#include <QColor>
#include <unordered_map>
#include <forward_list>

//! Class for calculation of a 3D Octree 
class iAVROctree
{
public:
	iAVROctree(vtkRenderer* ren, vtkDataSet* dataSet);
	void generateOctreeRepresentation(int level, QColor col);
	void calculateOctree(int level, int pointsPerRegion);
	void setDataset(vtkDataSet* dataSet);
	vtkOctreePointLocator* getOctree();
	void calculateOctreeRegionSize(int regionID, double size[3]);
	void calculateOctreeCenterPos(double centerPoint[3]);
	void calculateOctreeRegionCenterPos(int regionID, double centerPoint[3]);
	void createOctreeBoundingBoxPlanes(int regionID, std::vector<std::vector<iAVec3d>>* planePoints);
	void createOctreeBoundingBoxPlanes(std::vector<std::vector<iAVec3d>>* planePoints);
	void movePointInsideRegion(double point[3], double movedPoint[3]);
	vtkIdType getNumberOfLeafeNodes();
	double getMaxDistanceOctCenterToRegionCenter();
	double getMaxDistanceOctCenterToFiber();
	std::vector<std::unordered_map<vtkIdType, double>*>* getfibersInRegionMapping(std::unordered_map<vtkIdType, vtkIdType>* pointIDToCsvIndex);
	int getLevel();
	vtkSmartPointer<vtkPolyData> getBoundingBoxes();
	void show();
	void hide();
	vtkActor* getActor();
	std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>>* getRegionsInLineOfRay();

private:
	vtkIdType numberOfLeaveNodes;
	double m_maxDistanceOctCenterToRegionCenter;
	double m_maxDistanceOctCenterToFiber;
	bool m_visible;
	int m_level;
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkDataSet> m_dataSet;
	vtkSmartPointer<vtkActor> m_actor;
	vtkSmartPointer<vtkOctreePointLocator> m_octree;
	vtkSmartPointer<vtkPolyData> m_boundingBoxes;
	//Saves the octree [region] and a  map of its fiber IDs [iD] with their coverage (0.0-1.0)
	std::vector<std::unordered_map<vtkIdType, double>*>* m_fibersInRegion;
	//Saves a 2D side view of an [direction] (x,y,z) of all regions which get hit by a ray traversing through every row/column [column][row] (2^level x 2^level Grid)
	std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>>* m_regionsInLine;

	void mapFibersToRegion(std::unordered_map<vtkIdType, vtkIdType>* pointIDToCsvIndex);
	double calculateDistanceOctCenterToRegionCenter();
	double calculateDistanceOctCenterToFiber();
	void calculateRayThroughCubeRow();
};
