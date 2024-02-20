// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAVec3.h>

#include <vtkSmartPointer.h>

#include <QColor>

#include <forward_list>
#include <unordered_map>

class vtkActor;
class vtkDataSet;
class vtkOctreePointLocator;
class vtkPolyData;
class vtkRenderer;

//! Class for calculation of a 3D Octree
class iAVROctree
{
public:
	iAVROctree(vtkRenderer* ren, vtkDataSet* dataSet);
	void generateOctreeRepresentation(int level, QColor col);
	//! Calculates the octree for the current level with the given amount of max points in a region
	void calculateOctree(int level, int pointsPerRegion);
	void setDataset(vtkDataSet* dataSet);
	vtkOctreePointLocator* getOctree();
	//! Calculates the size of Octree leaf node
	void calculateOctreeRegionSize(int regionID, double size[3]);
	//! Calculates the center point between max and min bound of the whole octree.
	void calculateOctreeCenterPos(double centerPoint[3]);
	//! Calculates the center point between max and min bound of a leaf region.
	void calculateOctreeRegionCenterPos(int regionID, double centerPoint[3]);
	//! Creates the six Planes defines by the octree bounds of the given leaf region.
	//! @param planePoints defined as six [planes] (0-5) with respectively 3 points (origin, point 1, point 2)
	void createOctreeBoundingBoxPlanes(int regionID, std::vector<std::vector<iAVec3d>> & planePoints);
	//! Creates the six Planes defined by the bounds of the whole octree.
	//! @param planePoints defined as six [planes] (0-5) with respectively 3 points (origin, point 1, point 2)
	void createOctreeBoundingBoxPlanes(std::vector<std::vector<iAVec3d>> & planePoints);
	//! Used for points which are just *barely* outside the bounds of the region. Moves that point so that it is just barely *inside* the bounds
	void movePointInsideRegion(double point[3], double movedPoint[3]);
	vtkIdType getNumberOfLeafNodes() const;
	//! Returns the maximum lenght from the octree center to any of its regions. Gets calculated at first call
	double getMaxDistanceOctCenterToRegionCenter();
	double getMaxDistanceOctCenterToFiber();
	//! Returns which fibers (ID) lie in which region with coverage of 1. Gets calculated on the first call.
	//! They all lie (independent of their real coverage) to 100% inside the region
	std::vector<std::unordered_map<vtkIdType, double>*> const & getFibersInRegionMapping(std::unordered_map<vtkIdType, vtkIdType> const & pointIDToCsvIndex);
	int getLevel() const;
	vtkSmartPointer<vtkPolyData> getBoundingBoxes();
	void show();
	void hide();
	vtkActor* getActor();
	//! Returns a vector for each projection on an plane (x,y,z) and each grid cell beginning on lower left cell (0,0).
	//! Grid size is 2^L x 2^L x 2^L (L = Level of Octree).
	//! For each cell a list of all traversed cubes is stored. Attention: In case a cube could not be splitted in later octree levels
	//! the list will contain this region multiple times.
	std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>> const & getRegionsInLineOfRay();

private:
	vtkIdType m_numberOfLeafNodes;
	double m_maxDistanceOctCenterToRegionCenter;
	double m_maxDistanceOctCenterToFiber;
	bool m_visible;
	int m_level;
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkDataSet> m_dataSet;
	vtkSmartPointer<vtkActor> m_actor;
	vtkSmartPointer<vtkOctreePointLocator> m_octree;
	vtkSmartPointer<vtkPolyData> m_boundingBoxes;
	//! Saves the octree [region] and a  map of its fiber IDs [iD] with their coverage (0.0-1.0)
	std::vector<std::unordered_map<vtkIdType, double>*> m_fibersInRegion;
	//! Saves a 2D side view of an [direction] (x,y,z) of all regions which get hit by a ray traversing through every row/column [column][row] (2^level x 2^level Grid)
	std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>> m_regionsInLine;

	//! Stores which fibers lie in which region. Therfore all pointIds inside a region are taken and mapped to its fiber ID.
	//! They all lie (independent of their real coverage) to 100% inside the region
	//! Regions without fibers have no entry
	void mapFibersToRegion(std::unordered_map<vtkIdType, vtkIdType> const & pointIDToCsvIndex);
	double calculateDistanceOctCenterToRegionCenter();
	double calculateDistanceOctCenterToFiber();
	//! Calculates which cubes a ray would traverse in x, y, z direction in an straight line
	//! The corresponding vector consists of an vector for each projection on an plane (x,y,z) and each grid cell.
	//! The start grid cells depend on the origin Point of the Plane drawn in iAVROctreeMetrics.
	//! For each cell a list of all traversed cubes is stored. Attention: In case a cube could not be splitted in later octree levels
	//! the list will contain this region multiple times.
	void calculateRayThroughCubeRow();
};
