// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVROctree.h"

#include <vtkPolyDataMapper.h>
#include "vtkPointData.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkIdTypeArray.h"
#include <iALog.h>


iAVROctree::iAVROctree(vtkRenderer* ren, vtkDataSet* dataSet):m_renderer(ren),m_dataSet(dataSet),m_actor(vtkSmartPointer<vtkActor>::New()),
m_octree(vtkSmartPointer<vtkOctreePointLocator>::New())
{
	m_visible = false;
	numberOfLeaveNodes = 0;
	m_maxDistanceOctCenterToRegionCenter = -1;
	m_maxDistanceOctCenterToFiber = -1;
	m_fibersInRegion = new std::vector<std::unordered_map<vtkIdType, double>*>();
	m_regionsInLine = new std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>>();
}

void iAVROctree::generateOctreeRepresentation(int level, QColor col)
{
	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	m_octree->GenerateRepresentation(level, polydata);

	vtkSmartPointer<vtkPolyDataMapper> octreeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	octreeMapper->SetInputData(polydata);

	m_actor->SetMapper(octreeMapper);
	m_actor->GetProperty()->SetRepresentationToWireframe();
	m_actor->GetProperty()->SetColor(col.redF(), col.greenF(), col.blueF());
	m_actor->GetProperty()->SetLineWidth(5);
	m_actor->GetProperty()->RenderLinesAsTubesOn();
	//m_actor->PickableOff();
}

//! The Method calculates the octree for the current level with the given amount of max points in a region
void iAVROctree::calculateOctree(int level, int pointsPerRegion)
{
	m_octree->SetMaximumPointsPerRegion(pointsPerRegion); // The maximum number of points in a region/octant before it is subdivided
	m_octree->SetMaxLevel(level);
	m_octree->SetDataSet(m_dataSet);
	m_octree->BuildLocator();

	numberOfLeaveNodes = m_octree->GetNumberOfLeafNodes();
	m_level = level;
	m_boundingBoxes = vtkSmartPointer<vtkPolyData>::New();
	m_octree->GenerateRepresentation(m_level, m_boundingBoxes);
}

void iAVROctree::setDataset(vtkDataSet* dataSet)
{
	m_dataSet = dataSet;
}

vtkOctreePointLocator * iAVROctree::getOctree()
{
	return m_octree;
}

//! Calculates the size of Octree leaf node
void iAVROctree::calculateOctreeRegionSize(int regionID, double size[3])
{
	double bounds[6];

	if (m_octree->GetNumberOfLeafNodes() >= regionID)
	{
		m_octree->GetRegionBounds(regionID, bounds);
		size[0] = abs(bounds[1] - bounds[0]);
		size[1] = abs(bounds[3] - bounds[2]);
		size[2] = abs(bounds[5] - bounds[4]);
	}
}

//! This Method calculates the center point between max and min bound of the whole octree.
void iAVROctree::calculateOctreeCenterPos(double centerPoint[3])
{
	double bounds[6];

	m_octree->GetBounds(bounds);
	centerPoint[0] = bounds[0] + ((bounds[1] - bounds[0]) / 2);
	centerPoint[1] = bounds[2] + ((bounds[3] - bounds[2]) / 2);
	centerPoint[2] = bounds[4] + ((bounds[5] - bounds[4]) / 2);
}

//! This Method calculates the center point between max and min bound of a leaf region.
void iAVROctree::calculateOctreeRegionCenterPos(int regionID, double centerPoint[3])
{
	double bounds[6];

	m_octree->GetRegionBounds(regionID, bounds);
	centerPoint[0] = bounds[0] + ((bounds[1] - bounds[0]) / 2);
	centerPoint[1] = bounds[2] + ((bounds[3] - bounds[2]) / 2);
	centerPoint[2] = bounds[4] + ((bounds[5] - bounds[4]) / 2);
}

//! This Method creates the six Planes defines by the octree bounds of the given leaf region.
//! The vec is defined as six [planes] (0-5) with respectively 3 points (origin, point 1, point 2)
void iAVROctree::createOctreeBoundingBoxPlanes(int regionID, std::vector<std::vector<iAVec3d>>* planePoints)
{
	double bounds[6];
	m_octree->GetRegionBounds(regionID, bounds);

	double xMin = bounds[0];
	double xMax = bounds[1];
	double yMin = bounds[2];
	double yMax = bounds[3];
	double zMin = bounds[4];
	double zMax = bounds[5];

	auto tempPos = std::vector<iAVec3d>();
	tempPos.reserve(3);
	planePoints->reserve(6);

	//Plane 1
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 2
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMax, yMax, zMax)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 3
	tempPos.push_back(iAVec3d(xMin, yMin, zMin)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMin)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 4
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMin, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 5
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 6
	tempPos.push_back(iAVec3d(xMax, yMax, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMax, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2 
	planePoints->push_back(tempPos);
}

//! This Method creates the six Planes defined by the bounds of the whole octree .
//! The vec is defined as six [planes] (0-5) with respectively 3 points (origin, point 1, point 2)
void iAVROctree::createOctreeBoundingBoxPlanes(std::vector<std::vector<iAVec3d>>* planePoints)
{
	double bounds[6];
	m_octree->GetBounds(bounds);

	double xMin = bounds[0];
	double xMax = bounds[1];
	double yMin = bounds[2];
	double yMax = bounds[3];
	double zMin = bounds[4];
	double zMax = bounds[5];

	auto tempPos = std::vector<iAVec3d>();
	tempPos.reserve(3);
	planePoints->reserve(6);

	//Plane 1
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 2
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMax, yMax, zMax)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 3
	tempPos.push_back(iAVec3d(xMin, yMin, zMin)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMin)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 4
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMin, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 5
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //point 2 
	planePoints->push_back(tempPos);
	tempPos.clear();

	//Plane 6
	tempPos.push_back(iAVec3d(xMax, yMax, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMax, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2 
	planePoints->push_back(tempPos);
}

//! Used for points which are just *barely* outside the bounds of the region. Moves that point so that it is just barely *inside* the bounds
void iAVROctree::movePointInsideRegion(double point[3], double movedPoint[3])
{
	double outerBounds[6];
	m_octree->GetBounds(outerBounds);

	movedPoint[0] = point[0];
	movedPoint[1] = point[1];
	movedPoint[2] = point[2];

	if (point[0] < outerBounds[0])
	{
		movedPoint[0] = point[0] + m_octree->GetFudgeFactor();
	}
	if (point[0] > outerBounds[1])
	{
		movedPoint[0] = point[0] - m_octree->GetFudgeFactor();
	}
	if (point[1] < outerBounds[2])
	{
		movedPoint[1] = point[1] + m_octree->GetFudgeFactor();
	}
	if (point[1] > outerBounds[3])
	{
		movedPoint[1] = point[1] - m_octree->GetFudgeFactor();
	}
	if (point[2] < outerBounds[4])
	{
		movedPoint[2] = point[2] + m_octree->GetFudgeFactor();
	}
	if (point[2] > outerBounds[5])
	{
		movedPoint[2] = point[2] - m_octree->GetFudgeFactor();
	}
}

vtkIdType iAVROctree::getNumberOfLeafeNodes()
{
	return numberOfLeaveNodes;
}

//! Returns the maximum lenght from the octree center to any of its regions. Gets calculated at first call
double iAVROctree::getMaxDistanceOctCenterToRegionCenter()
{
	if(m_maxDistanceOctCenterToRegionCenter != -1)
	{
		return m_maxDistanceOctCenterToRegionCenter;
	}
	else
	{
		m_maxDistanceOctCenterToRegionCenter = calculateDistanceOctCenterToRegionCenter();
		return m_maxDistanceOctCenterToRegionCenter;
	}	
}

double iAVROctree::getMaxDistanceOctCenterToFiber()
{
	if (m_maxDistanceOctCenterToFiber != -1)
	{
		return m_maxDistanceOctCenterToFiber;
	}
	else
	{
		m_maxDistanceOctCenterToFiber = calculateDistanceOctCenterToRegionCenter();
		return m_maxDistanceOctCenterToFiber;
	}
}

//! Returns which fibers (ID) lie in which region with coverage of 1. Gets calculated on the first call.
//! They all lie (independent of their real coverage) to 100% inside the region
std::vector<std::unordered_map<vtkIdType, double>*>* iAVROctree::getfibersInRegionMapping(std::unordered_map<vtkIdType, vtkIdType>* pointIDToCsvIndex)
{
	if(!m_fibersInRegion->empty())
	{
		return m_fibersInRegion;
	}
	else
	{
		mapFibersToRegion(pointIDToCsvIndex);
		return m_fibersInRegion;
	}
}

int iAVROctree::getLevel()
{
	return m_level;
}

vtkSmartPointer<vtkPolyData> iAVROctree::getBoundingBoxes()
{
	return m_boundingBoxes;
}

void iAVROctree::show()
{
	if (m_visible)
	{
		return;
	}
	m_renderer->AddActor(m_actor);
	m_visible = true;
}

void iAVROctree::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_renderer->RemoveActor(m_actor);
	m_visible = false;
}

vtkActor* iAVROctree::getActor()
{
	return m_actor;
}

//! Returns a vector for each projection on an plane (x,y,z) and each grid cell beginning on lower left cell (0,0).
//! Grid size is 2^L x 2^L x 2^L (L = Level of Octree).
//! For each cell a list of all traversed cubes is stored. Attention: In case a cube could not be splitted in later octree levels 
//! the list will contain this region multiple times.
std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>>* iAVROctree::getRegionsInLineOfRay()
{
	if (!m_regionsInLine->empty())
	{
		return m_regionsInLine;
	}
	else
	{
		calculateRayThroughCubeRow();
		return m_regionsInLine;
	}
}

//! Stores which fibers lie in which region. Therfore all pointIds inside a region are taken and mapped to its fiber ID. 
//! They all lie (independent of their real coverage) to 100% inside the region
//! Regions without fibers have no entry
void iAVROctree::mapFibersToRegion(std::unordered_map<vtkIdType, vtkIdType>* pointIDToCsvIndex)
{
	for(int region = 0; region < numberOfLeaveNodes; region++)
	{
		vtkIdTypeArray* points = m_octree->GetPointsInRegion(region);
		std::unordered_map<vtkIdType, double>* tempMap = new std::unordered_map<vtkIdType, double>();
		//Check if points is not null!!
		if (points != nullptr) {

			for (vtkIdType i = 0; i < points->GetSize(); i++)
			{
				vtkIdType fiberiD;
				if (pointIDToCsvIndex->find(points->GetValue(i)) != pointIDToCsvIndex->end())
				{
					fiberiD = pointIDToCsvIndex->at(points->GetValue(i));
				}
				tempMap->insert(std::make_pair(fiberiD, 1));
			}
		}
		m_fibersInRegion->push_back(tempMap);
	}
}

double iAVROctree::calculateDistanceOctCenterToRegionCenter()
{
	double maxLength = 0;
	double centerPoint[3];
	double regionCenterPoint[3];
	calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	for (vtkIdType region = 0; region < getNumberOfLeafeNodes(); region++)
	{
		calculateOctreeRegionCenterPos(region, regionCenterPoint);

		iAVec3d currentRegionCenterPoint = iAVec3d(regionCenterPoint);
		iAVec3d direction = currentRegionCenterPoint - centerPos;
		double length = direction.length();
		if (length > maxLength) maxLength = length;		// Get max length
	}

	return maxLength;
}

double iAVROctree::calculateDistanceOctCenterToFiber()
{
	double maxLength = 0;
	double centerPoint[3];
	calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	// Get max length
	for (vtkIdType i = 0; i < m_octree->GetDataSet()->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(m_octree->GetDataSet()->GetPoint(i));
		iAVec3d direction = currentPoint - centerPos;
		double length = direction.length();

		if (length > maxLength) maxLength = length;
	}

	return maxLength;
}

//! Calculates which cubes a ray would traverse in x, y, z direction in an straight line
//! The corresponding vector consists of an vector for each projection on an plane (x,y,z) and each grid cell.
//! The start grid cells depend on the origin Point of the Plane drawn in iAVROctreeMetrics.
//! For each cell a list of all traversed cubes is stored. Attention: In case a cube could not be splitted in later octree levels 
//! the list will contain this region multiple times.
void iAVROctree::calculateRayThroughCubeRow()
{
	double bounds[6];
	m_octree->GetBounds(bounds);

	int rowSize = pow(2, getLevel());
	int rowSizeMinus1 = rowSize - 1;
	std::vector<std::forward_list<vtkIdType>> row = std::vector<std::forward_list<vtkIdType>>(rowSize);
	std::vector<std::vector<std::forward_list<vtkIdType>>> column = std::vector<std::vector<std::forward_list<vtkIdType>>>(rowSize, row);
	m_regionsInLine = new std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>>(3, column);

	double xMin = bounds[0];
	double xMax = bounds[1];
	double xRange = xMax - xMin;
	double xSteps = xRange / rowSize;

	double yMin = bounds[2];
	double yMax = bounds[3];
	double yRange = yMax - yMin;
	double ySteps = yRange / rowSize;

	double zMin = bounds[4];
	double zMax = bounds[5];
	double zRange = zMax - zMin;
	double zSteps = zRange / rowSize;

	int columnCount = 0;
	for (int y = yMin + ySteps; y < yMax; y += ySteps)
	{
		int rowCount = 0;
		for (int x = xMin + xSteps; x < xMax; x += xSteps)
		{
			int depthCount = 0;

			for (int z = zMin + zSteps; z < zMax; z += zSteps)
			{
				vtkIdType region = m_octree->GetRegionContainingPoint(x - (xSteps / 2), y - (ySteps / 2), z - (zSteps / 2));

				//x direction
				m_regionsInLine->at(0).at(columnCount).at(rowCount).push_front(region); 
				//y direction
				m_regionsInLine->at(1).at(rowSizeMinus1 - rowCount).at(rowSizeMinus1 - depthCount).push_front(region); 
				//z direction
				m_regionsInLine->at(2).at(columnCount).at(rowSizeMinus1 - depthCount).push_front(region);
				depthCount++;
			}	
			rowCount++;
		}
		columnCount++;
	}
}
