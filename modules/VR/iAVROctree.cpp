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
#include "iAVROctree.h"

#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkIdTypeArray.h"
#include <iAConsole.h>

iAVROctree::iAVROctree(vtkRenderer* ren, vtkDataSet* dataSet):m_renderer(ren),m_dataSet(dataSet),m_actor(vtkSmartPointer<vtkActor>::New()),
m_octree(vtkSmartPointer<vtkOctreePointLocator>::New())
{
	m_visible = false;
	numberOfLeaveNodes = 0;
	m_fibersInRegion = new std::vector<std::unordered_map<vtkIdType, double>*>();
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
	m_actor->GetProperty()->SetLineWidth(6);
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
}

vtkOctreePointLocator * iAVROctree::getOctree()
{
	return m_octree;
}

//! Calculates the size of the first (= 0) Octree leaf node
void iAVROctree::calculateOctreeRegionSize(double size[3])
{
	double bounds[6];

	if (m_octree->GetNumberOfLeafNodes() > 0)
	{
		m_octree->GetRegionBounds(0, bounds);
		size[0] = abs(bounds[1] - bounds[0]);
		size[1] = abs(bounds[3] - bounds[2]);
		size[2] = abs(bounds[5] - bounds[4]);
	}
}

//! This Method calculates the center point between upper and lower bound of a leaf region.
void iAVROctree::calculateOctreeRegionCenterPos(int regionID, double centerPoint[3])
{
	double bounds[6];

	m_octree->GetRegionBounds(regionID, bounds);
	centerPoint[0] = bounds[0] + ((bounds[1] - bounds[0]) / 2);
	centerPoint[1] = bounds[2] + ((bounds[3] - bounds[2]) / 2);
	centerPoint[2] = bounds[4] + ((bounds[5] - bounds[4]) / 2);
}

//! This Method creates the six Planes defines by the octree bounds of the given leaf region.
void iAVROctree::createOctreeBoundingBoxPlanes(int regionID, std::vector<vtkSmartPointer<vtkPlaneSource>>* planes)
{
	double bounds[6];
	m_octree->GetRegionBounds(regionID, bounds);

	double xMin = bounds[0];
	double xMax = bounds[1];
	double yMin = bounds[2];
	double yMax = bounds[3];
	double zMin = bounds[4];
	double zMax = bounds[5];

	vtkSmartPointer<vtkPlaneSource> planeSource0 = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource0->SetOrigin(xMin, yMin, zMax);
	planeSource0->SetPoint1(xMax, yMin, zMax);
	planeSource0->SetPoint2(xMin, yMax, zMax);
	planeSource0->Update();
	planes->push_back(planeSource0);

	vtkSmartPointer<vtkPlaneSource> planeSource1 = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource1->SetOrigin(xMax, yMin, zMax);
	planeSource1->SetPoint1(xMax, yMin, zMin);
	planeSource1->SetPoint2(xMax, yMax, zMax);
	planeSource1->Update();
	planes->push_back(planeSource1);

	vtkSmartPointer<vtkPlaneSource> planeSource2 = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource2->SetOrigin(xMin, yMin, zMin);
	planeSource2->SetPoint1(xMax, yMin, zMin);
	planeSource2->SetPoint2(xMin, yMax, zMin);
	planeSource2->Update();
	planes->push_back(planeSource2);

	vtkSmartPointer<vtkPlaneSource> planeSource3 = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource3->SetOrigin(xMin, yMin, zMax);
	planeSource3->SetPoint1(xMin, yMin, zMin);
	planeSource3->SetPoint2(xMin, yMax, zMax);
	planeSource3->Update();
	planes->push_back(planeSource3);

	vtkSmartPointer<vtkPlaneSource> planeSource4 = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource4->SetOrigin(xMax, yMin, zMax);
	planeSource4->SetPoint1(xMax, yMin, zMin);
	planeSource4->SetPoint2(xMin, yMin, zMax);
	planeSource4->Update();
	planes->push_back(planeSource4);

	vtkSmartPointer<vtkPlaneSource> planeSource5 = vtkSmartPointer<vtkPlaneSource>::New();
	planeSource5->SetOrigin(xMax, yMax, zMax);
	planeSource5->SetPoint1(xMax, yMax, zMin);
	planeSource5->SetPoint2(xMin, yMax, zMax);
	planeSource5->Update();
	planes->push_back(planeSource5);

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

int iAVROctree::getNumberOfLeafeNodes()
{
	return numberOfLeaveNodes;
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

//! Stores which fibers lie in which region
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

			for (int i = 0; i < points->GetSize(); i++)
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
