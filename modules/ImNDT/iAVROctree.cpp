// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVROctree.h"

#include <vtkActor.h>
#include <vtkIdTypeArray.h>
#include <vtkOctreePointLocator.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

iAVROctree::iAVROctree(vtkRenderer* ren, vtkDataSet* dataSet):
	m_numberOfLeafNodes(0),
	m_maxDistanceOctCenterToRegionCenter(-1),
	m_maxDistanceOctCenterToFiber(-1),
	m_visible(false),
	m_level(-1),
	m_renderer(ren),
	m_dataSet(dataSet),
	m_actor(vtkSmartPointer<vtkActor>::New()),
	m_octree(vtkSmartPointer<vtkOctreePointLocator>::New()),
	m_fibersInRegion(),
	m_regionsInLine()
{
}

void iAVROctree::generateOctreeRepresentation(int level, QColor col)
{
	vtkNew<vtkPolyData> polydata;
	m_octree->GenerateRepresentation(level, polydata);

	vtkNew<vtkPolyDataMapper> octreeMapper;
	octreeMapper->SetInputData(polydata);

	m_actor->SetMapper(octreeMapper);
	m_actor->GetProperty()->SetRepresentationToWireframe();
	m_actor->GetProperty()->SetColor(col.redF(), col.greenF(), col.blueF());
	m_actor->GetProperty()->SetLineWidth(5);
	m_actor->GetProperty()->RenderLinesAsTubesOn();
	//m_actor->PickableOff();
}

void iAVROctree::calculateOctree(int level, int pointsPerRegion)
{
	m_octree->SetMaximumPointsPerRegion(pointsPerRegion); // The maximum number of points in a region/octant before it is subdivided
	m_octree->SetMaxLevel(level);
	m_octree->SetDataSet(m_dataSet);
	m_octree->BuildLocator();
	m_numberOfLeafNodes = m_octree->GetNumberOfLeafNodes();
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

void iAVROctree::calculateOctreeRegionSize(int regionID, double size[3])
{
	if (m_octree->GetNumberOfLeafNodes() >= regionID)
	{
		double bounds[6];
		m_octree->GetRegionBounds(regionID, bounds);
		size[0] = abs(bounds[1] - bounds[0]);
		size[1] = abs(bounds[3] - bounds[2]);
		size[2] = abs(bounds[5] - bounds[4]);
	}
}

void iAVROctree::calculateOctreeCenterPos(double centerPoint[3])
{
	double bounds[6];
	m_octree->GetBounds(bounds);
	centerPoint[0] = bounds[0] + ((bounds[1] - bounds[0]) / 2);
	centerPoint[1] = bounds[2] + ((bounds[3] - bounds[2]) / 2);
	centerPoint[2] = bounds[4] + ((bounds[5] - bounds[4]) / 2);
}

void iAVROctree::calculateOctreeRegionCenterPos(int regionID, double centerPoint[3])
{
	double bounds[6];
	m_octree->GetRegionBounds(regionID, bounds);
	centerPoint[0] = bounds[0] + ((bounds[1] - bounds[0]) / 2);
	centerPoint[1] = bounds[2] + ((bounds[3] - bounds[2]) / 2);
	centerPoint[2] = bounds[4] + ((bounds[5] - bounds[4]) / 2);
}

void iAVROctree::createOctreeBoundingBoxPlanes(int regionID, std::vector<std::vector<iAVec3d>> & planePoints)
{
	double bounds[6];
	m_octree->GetRegionBounds(regionID, bounds);

	double xMin = bounds[0];
	double xMax = bounds[1];
	double yMin = bounds[2];
	double yMax = bounds[3];
	double zMin = bounds[4];
	double zMax = bounds[5];

	std::vector<iAVec3d> tempPos;
	tempPos.reserve(3);
	planePoints.reserve(6);

	//Plane 1
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 2
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMax, yMax, zMax)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 3
	tempPos.push_back(iAVec3d(xMin, yMin, zMin)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMin)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 4
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMin, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 5
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 6
	tempPos.push_back(iAVec3d(xMax, yMax, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMax, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2
	planePoints.push_back(tempPos);
}

void iAVROctree::createOctreeBoundingBoxPlanes(std::vector<std::vector<iAVec3d>> & planePoints)
{
	double bounds[6];
	m_octree->GetBounds(bounds);

	double xMin = bounds[0];
	double xMax = bounds[1];
	double yMin = bounds[2];
	double yMax = bounds[3];
	double zMin = bounds[4];
	double zMax = bounds[5];

	std::vector<iAVec3d> tempPos;
	tempPos.reserve(3);
	planePoints.reserve(6);

	//Plane 1
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 2
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMax, yMax, zMax)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 3
	tempPos.push_back(iAVec3d(xMin, yMin, zMin)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMin)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 4
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMin, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 5
	tempPos.push_back(iAVec3d(xMax, yMin, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMin, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMin, zMax)); //point 2
	planePoints.push_back(tempPos);
	tempPos.clear();

	//Plane 6
	tempPos.push_back(iAVec3d(xMax, yMax, zMax)); //origin
	tempPos.push_back(iAVec3d(xMax, yMax, zMin)); //point 1
	tempPos.push_back(iAVec3d(xMin, yMax, zMax)); //point 2
	planePoints.push_back(tempPos);
}

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

vtkIdType iAVROctree::getNumberOfLeafNodes() const
{
	return m_numberOfLeafNodes;
}

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

std::vector<std::unordered_map<vtkIdType, double>*> const & iAVROctree::getFibersInRegionMapping(std::unordered_map<vtkIdType, vtkIdType> const & pointIDToCsvIndex)
{
	if(!m_fibersInRegion.empty())
	{
		return m_fibersInRegion;
	}
	else
	{
		mapFibersToRegion(pointIDToCsvIndex);
		return m_fibersInRegion;
	}
}

int iAVROctree::getLevel() const
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

std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>> const & iAVROctree::getRegionsInLineOfRay()
{
	if (!m_regionsInLine.empty())
	{
		return m_regionsInLine;
	}
	else
	{
		calculateRayThroughCubeRow();
		return m_regionsInLine;
	}
}

void iAVROctree::mapFibersToRegion(std::unordered_map<vtkIdType, vtkIdType> const & pointIDToCsvIndex)
{
	for(int region = 0; region < m_numberOfLeafNodes; region++)
	{
		vtkIdTypeArray* points = m_octree->GetPointsInRegion(region);
		std::unordered_map<vtkIdType, double>* tempMap = new std::unordered_map<vtkIdType, double>();
		//Check if points is not null!!
		if (points != nullptr) {

			for (vtkIdType i = 0; i < points->GetSize(); i++)
			{
				vtkIdType fiberiD = -1;
				if (pointIDToCsvIndex.contains(points->GetValue(i)))
				{
					fiberiD = pointIDToCsvIndex.at(points->GetValue(i));
				}
				tempMap->insert(std::make_pair(fiberiD, 1));
			}
		}
		m_fibersInRegion.push_back(tempMap);
	}
}

double iAVROctree::calculateDistanceOctCenterToRegionCenter()
{
	double maxLength = 0;
	double centerPoint[3];
	double regionCenterPoint[3];
	calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	for (vtkIdType region = 0; region < getNumberOfLeafNodes(); region++)
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

void iAVROctree::calculateRayThroughCubeRow()
{
	double bounds[6];
	m_octree->GetBounds(bounds);

	int rowSize = pow(2, getLevel());
	int rowSizeMinus1 = rowSize - 1;
	std::vector<std::forward_list<vtkIdType>> row = std::vector<std::forward_list<vtkIdType>>(rowSize);
	std::vector<std::vector<std::forward_list<vtkIdType>>> column = std::vector<std::vector<std::forward_list<vtkIdType>>>(rowSize, row);
	m_regionsInLine.resize(3, column);

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
				m_regionsInLine.at(0).at(columnCount).at(rowCount).push_front(region);
				//y direction
				m_regionsInLine.at(1).at(rowSizeMinus1 - rowCount).at(rowSizeMinus1 - depthCount).push_front(region);
				//z direction
				m_regionsInLine.at(2).at(columnCount).at(rowSizeMinus1 - depthCount).push_front(region);
				depthCount++;
			}
			rowCount++;
		}
		columnCount++;
	}
}
