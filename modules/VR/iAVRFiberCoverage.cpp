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

#include "iAVRFiberCoverage.h"

#include <iALog.h>

iAVRFiberCoverage::iAVRFiberCoverage(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees, iAVRVolume* volume) : m_objectTable(objectTable), m_io(io), 
m_octrees(octrees), m_volume(volume)
{
	m_fiberCoverage = new std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>();
}

//! Computes which polyObject ID (points) belongs to which Object ID in the csv file of the volume
//! Gets only called internally from thread to store the mapping
void iAVRFiberCoverage::mapAllPointiDs()
{
	// For every fiber in csv table
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		double startPos[3]{}, endPos[3]{};
		for (int k = 0; k < 3; ++k)
		{
			startPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::StartX + k)).ToFloat();
			endPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::EndX + k)).ToFloat();
		}

		// Insert polyObject ID of Start Point and End Point
		m_pointIDToCsvIndex.insert(std::make_pair(m_volume->getVolumeData()->FindPoint(startPos), row));
		m_pointIDToCsvIndex.insert(std::make_pair(m_volume->getVolumeData()->FindPoint(endPos), row));

		// Insert fiber id with its Start Point and End Point
		m_csvIndexToPointID.insert(std::make_pair(row, m_volume->getVolumeData()->FindPoint(startPos)));
		m_csvIndexToPointID.insert(std::make_pair(row, m_volume->getVolumeData()->FindPoint(endPos)));
	}
	LOG(lvlInfo, QString("Volume Data loaded"));

	//Calculate Fibers in Region
	for (size_t i = 0; i < m_octrees->size(); i++)
	{
		m_fiberCoverage->push_back(*m_octrees->at(i)->getfibersInRegionMapping(&m_pointIDToCsvIndex));
	}
}

//! Computes which polyObject ID (points) belongs to which Object ID in the csv file of the volume for every Octree Level
//! Calculates Intersection between points in different Octree regions in all levels. Calls a emthod to compute fiber coverage.
//! Gets only called internally
void iAVRFiberCoverage::mapAllPointiDsAndCalculateFiberCoverage()
{
	int count = 0;

	//Initialize new Vectors
	for (size_t level = 0; level < m_octrees->size(); level++)
	{
		//Initialize the region vec for every level
		m_fiberCoverage->push_back(std::vector<std::unordered_map<vtkIdType, double>*>());

		for (int i = 0; i < m_octrees->at(level)->getNumberOfLeafeNodes(); i++)
		{
			//Initialize a vec of Maps for every region
			m_fiberCoverage->at(level).push_back(new std::unordered_map<vtkIdType, double>());
		}
	}

	// For every fiber in csv table
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		double startPos[3]{}, endPos[3]{};
		for (int k = 0; k < 3; ++k)
		{
			startPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::StartX + k)).ToFloat();
			endPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::EndX + k)).ToFloat();
		}

		// Insert polyObject ID of Start Point and End Point
		m_pointIDToCsvIndex.insert(std::make_pair(m_volume->getVolumeData()->FindPoint(startPos), row));
		m_pointIDToCsvIndex.insert(std::make_pair(m_volume->getVolumeData()->FindPoint(endPos), row));

		// Insert fiber id with its Start Point and End Point
		m_csvIndexToPointID.insert(std::make_pair(row, m_volume->getVolumeData()->FindPoint(startPos)));
		m_csvIndexToPointID.insert(std::make_pair(row, m_volume->getVolumeData()->FindPoint(endPos)));

		vtkSmartPointer<vtkPoints> intersectionPoints = vtkSmartPointer<vtkPoints>::New();

		//For every Octree Level
		//for (int level = OCTREE_MIN_LEVEL; level <= 1; level++)
		for (size_t level = 0; level < m_octrees->size(); level++)
		{
			//Skip intersection test on lowest Octree level
			if (level == 0)
			{
				//Every fiber is 100% in the one region
				m_fiberCoverage->at(0).at(0)->insert(std::make_pair(row, 1.0));
			}
			else
			{
				double fiberLength = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::Length)).ToFloat();
				//std::vector<std::unordered_map<vtkIdType, double>*> coverageInRegion = std::vector<std::unordered_map<vtkIdType, double>*>();
				//m_fiberCoverage->push_back(coverageInRegion);

				intersectionPoints = getOctreeFiberCoverage(startPos, endPos, level, row, fiberLength);
				count += intersectionPoints->GetNumberOfPoints();

				if (intersectionPoints == nullptr)
				{
					LOG(lvlDebug, QString("!! vtkPoints is null..."));
				}
			}
		}
	}

	for (size_t level = 1; level < m_octrees->size(); level++)
	{
		m_octrees->at(level)->getRegionsInLineOfRay();
	}

	LOG(lvlInfo, QString("Volume Data processed and loaded"));
}

std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* iAVRFiberCoverage::getFiberCoverage()
{
	return m_fiberCoverage;
}

//! Returns the iD (row of csv) of the fiber corresponding to the polyObject ID
//! Returns -1 if point is not found in csv
vtkIdType iAVRFiberCoverage::getObjectiD(vtkIdType polyPoint)
{
	if (m_pointIDToCsvIndex.find(polyPoint) != m_pointIDToCsvIndex.end())
	{
		return m_pointIDToCsvIndex.at(polyPoint);
	}
	else
	{
		LOG(lvlDebug, QString("Point ID not found in csv"));
		return -1;
	}
}

std::unordered_map<vtkIdType, vtkIdType> iAVRFiberCoverage::getPointIDToCsvIndexMapper()
{
	return m_pointIDToCsvIndex;
}

std::unordered_multimap<vtkIdType, vtkIdType> iAVRFiberCoverage::getCsvIndexToPointIDMapper()
{
	return m_csvIndexToPointID;
}

//! The calculated intersection points are returned as vtkPoints
vtkSmartPointer<vtkPoints> iAVRFiberCoverage::getOctreeFiberCoverage(double startPoint[3], double endPoint[3], vtkIdType octreeLevel, vtkIdType fiber, double fiberLength)
{
	vtkSmartPointer<vtkPoints> additionalIntersectionPoints = vtkSmartPointer<vtkPoints>::New();

	//m_octree->calculateOctree(octreeLevel, OCTREE_POINTS_PER_REGION);
	vtkIdType leafNodes = m_octrees->at(octreeLevel)->getNumberOfLeafeNodes();
	vtkIdType startPointInsideRegion = m_octrees->at(octreeLevel)->getOctree()->GetRegionContainingPoint(startPoint[0], startPoint[1], startPoint[2]);
	vtkIdType endPointInsideRegion = m_octrees->at(octreeLevel)->getOctree()->GetRegionContainingPoint(endPoint[0], endPoint[1], endPoint[2]);
	// Sometimes Point is *barely* outside the bounds of the region ->move them in to check region
	if (startPointInsideRegion == -1)
	{
		double insideStartPoint[3];
		m_octrees->at(octreeLevel)->movePointInsideRegion(startPoint, insideStartPoint);
		startPointInsideRegion = m_octrees->at(octreeLevel)->getOctree()->GetRegionContainingPoint(insideStartPoint[0], insideStartPoint[1], insideStartPoint[2]);
	}
	if (endPointInsideRegion == -1)
	{
		double insideEndPoint[3];
		m_octrees->at(octreeLevel)->movePointInsideRegion(endPoint, insideEndPoint);
		endPointInsideRegion = m_octrees->at(octreeLevel)->getOctree()->GetRegionContainingPoint(insideEndPoint[0], insideEndPoint[1], insideEndPoint[2]);
	}

	for (vtkIdType region = 0; region < leafNodes; region++)
	{
		double lastIntersection[3] = { -1, -1, -1 };
		int pointsInRegion = 0;
		double bounds[6];
		//std::vector<std::vector<iAVec3d>>* planePoints = new std::vector<std::vector<iAVec3d>>();
		//m_octrees->at(octreeLevel)->createOctreeBoundingBoxPlanes(region, planePoints);
		m_octrees->at(octreeLevel)->getOctree()->GetRegionBounds(region, bounds);

		//The ray between start to endpoint can only intersect 2 times with a octree region bounding box
		while (pointsInRegion < 2)
		{
			double intersectionPoint[3] = { -1, -1, -1 };

			if (startPointInsideRegion == region && endPointInsideRegion == region)
			{
				//double coverage = calculateFiberCoverage(startPoint, endPoint, fiberLength);
				m_fiberCoverage->at(octreeLevel).at(region)->insert(std::make_pair(fiber, 1.0));
				pointsInRegion = 2;
				return additionalIntersectionPoints; // whole fiber is in one region -> no intersection possible
			}
			else if (startPointInsideRegion == region)
			{
				if (checkIntersectionWithBox(startPoint, endPoint, bounds, intersectionPoint))
				{
					additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
					double coverage = calculateFiberCoverage(startPoint, intersectionPoint, fiberLength);
					pointsInRegion = 2;
					m_fiberCoverage->at(octreeLevel).at(region)->insert(std::make_pair(fiber, coverage));
					break;
				}
				else
				{
					// startpoint and Intersection point are the same position (lie on same boundary but are in different regions for the octree)
					break;
				}
			}
			else if (endPointInsideRegion == region)
			{
				if (checkIntersectionWithBox(endPoint, startPoint, bounds, intersectionPoint))
				{
					additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
					double coverage = calculateFiberCoverage(intersectionPoint, endPoint, fiberLength);
					pointsInRegion = 2;
					m_fiberCoverage->at(octreeLevel).at(region)->insert(std::make_pair(fiber, coverage));
					break;
				}
				else
				{
					// endPoint and Intersection point are the same position (lie on same boundary but are in different regions for the octree)
					break;
				}
			}
			else
			{
				//When a second point was found...
				if (pointsInRegion > 0)
				{
					if (checkIntersectionWithBox(lastIntersection, endPoint, bounds, intersectionPoint))
					{
						additionalIntersectionPoints->InsertNextPoint(lastIntersection);
						additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
						double coverage = calculateFiberCoverage(lastIntersection, intersectionPoint, fiberLength);
						pointsInRegion = 2;
						m_fiberCoverage->at(octreeLevel).at(region)->insert(std::make_pair(fiber, coverage));
						break;
					}
					else if (checkIntersectionWithBox(lastIntersection, startPoint, bounds, intersectionPoint))
					{
						additionalIntersectionPoints->InsertNextPoint(lastIntersection);
						additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
						double coverage = calculateFiberCoverage(lastIntersection, intersectionPoint, fiberLength);
						pointsInRegion = 2;
						m_fiberCoverage->at(octreeLevel).at(region)->insert(std::make_pair(fiber, coverage));
						break;
					}
					else
					{
						//Corner Point with no second point in region
						break;
					}
				}
				else // look if there is a intersection with a region outside of the startPoint/endPoint region
				{
					if (checkIntersectionWithBox(startPoint, endPoint, bounds, intersectionPoint))
					{
						pointsInRegion += 1;
						lastIntersection[0] = intersectionPoint[0];
						lastIntersection[1] = intersectionPoint[1];
						lastIntersection[2] = intersectionPoint[2];
						//Point gets saved if second point is found in this region
					}
					else
					{
						//There are no possible intersections
						break;
					}
				}
			}
		}
	}

	return additionalIntersectionPoints;
}

bool iAVRFiberCoverage::checkIntersectionWithBox(double startPoint[3], double endPoint[3], std::vector<std::vector<iAVec3d>>* planePoints, double bounds[6], double intersection[3])
{
	double eps = 0.00001;

	iAVec3d p0 = iAVec3d(startPoint);
	iAVec3d p1 = iAVec3d(endPoint);
	iAVec3d ray = p1 - p0;
	iAVec3d ray_normalized = p1 - p0;
	ray_normalized.normalize();

	for (int i = 0; i < 6; i++)
	{
		//Calculate Plane Normal (origin - point 1,2)
		iAVec3d pointOnPlaneOrigin = iAVec3d(planePoints->at(i).at(0));
		iAVec3d planeVec1 = planePoints->at(i).at(1) - pointOnPlaneOrigin;
		iAVec3d planeVec2 = planePoints->at(i).at(2) - pointOnPlaneOrigin;

		iAVec3d normal = crossProduct(planeVec1, planeVec2);
		normal.normalize();

		if (abs(dotProduct(normal, ray_normalized)) > eps) { // If false -> the line is parallel to the plane
			iAVec3d difference = pointOnPlaneOrigin - p0;

			// Compute the t value for the directed line ray intersecting the plane
			double t = dotProduct(difference, normal) / dotProduct(normal, ray_normalized);

			// possible intersection point in infinety
			intersection[0] = p0[0] + (ray_normalized[0] * t);
			intersection[1] = p0[1] + (ray_normalized[1] * t);
			intersection[2] = p0[2] + (ray_normalized[2] * t);

			// t has to be smaller or equal to length of ray and bigger then 0
			//Intersection must be in the inside the finite plane
			if ((t > 0) && (t <= ray.length()) &&
				(intersection[0] <= bounds[1]) && (intersection[0] >= bounds[0]) &&
				(intersection[1] <= bounds[3]) && (intersection[1] >= bounds[2]) &&
				(intersection[2] <= bounds[5]) && (intersection[2] >= bounds[4]))
			{
				return true;
			}
		}
	}
	return false;
}

bool iAVRFiberCoverage::checkIntersectionWithBox(double startPoint[3], double endPoint[3], double bounds[6], double intersection[3])
{
	double eps = 0.00001;

	iAVec3d p0 = iAVec3d(startPoint);
	iAVec3d p1 = iAVec3d(endPoint);
	iAVec3d ray = p1 - p0;
	iAVec3d ray_normalized = p1 - p0;
	ray_normalized.normalize();

	for (int i = 0; i < 6; i++)
	{
		iAVec3d pointOnPlaneOrigin = iAVec3d();
		iAVec3d planeP1 = iAVec3d();
		iAVec3d planeP2 = iAVec3d();

		createPlanePoint(i, bounds, &pointOnPlaneOrigin, &planeP1, &planeP2);

		//Calculate Plane Normal (origin - point 1,2)
		iAVec3d planeVec1 = planeP1 - pointOnPlaneOrigin;
		iAVec3d planeVec2 = planeP2 - pointOnPlaneOrigin;

		iAVec3d normal = crossProduct(planeVec1, planeVec2);
		normal.normalize();

		if (abs(dotProduct(normal, ray_normalized)) > eps) { // If false -> the line is parallel to the plane
			iAVec3d difference = pointOnPlaneOrigin - p0;

			// Compute the t value for the directed line ray intersecting the plane
			double t = dotProduct(difference, normal) / dotProduct(normal, ray_normalized);

			// possible intersection point in infinety
			intersection[0] = p0[0] + (ray_normalized[0] * t);
			intersection[1] = p0[1] + (ray_normalized[1] * t);
			intersection[2] = p0[2] + (ray_normalized[2] * t);

			// t has to be smaller or equal to length of ray and bigger then 0
			//Intersection must be in the inside the finite plane
			if ((t > 0) && (t <= ray.length()) &&
				(intersection[0] <= bounds[1]) && (intersection[0] >= bounds[0]) &&
				(intersection[1] <= bounds[3]) && (intersection[1] >= bounds[2]) &&
				(intersection[2] <= bounds[5]) && (intersection[2] >= bounds[4]))
			{
				return true;
			}
		}
	}
	return false;
}

double iAVRFiberCoverage::calculateFiberCoverage(double startPoint[3], double endPoint[3], double fiberLength)
{
	double vectorBetweenStartAndEnd[3]{};
	for (int i = 0; i < 3; i++)
	{
		vectorBetweenStartAndEnd[i] = (endPoint[i] - startPoint[i]);
	}

	double coverage = sqrt(pow(vectorBetweenStartAndEnd[0], 2) + pow(vectorBetweenStartAndEnd[1], 2) + pow(vectorBetweenStartAndEnd[2], 2));
	double ratio = coverage / fiberLength;

	return ratio;
}

//! Checks if two pos arrays are the same
bool iAVRFiberCoverage::checkEqualArrays(float pos1[3], float pos2[3])
{
	for (int i = 0; i < 3; ++i)
	{
		if (pos1[i] != pos2[i])
		{
			return false;
		}
	}
	return true;
}

bool iAVRFiberCoverage::checkEqualArrays(double pos1[3], double pos2[3])
{
	for (int i = 0; i < 3; ++i)
	{
		if (pos1[i] != pos2[i])
		{
			return false;
		}
	}
	return true;
}

void iAVRFiberCoverage::createPlanePoint(int plane, double bounds[6], iAVec3d* planeOrigin, iAVec3d* planeP1, iAVec3d* planeP2)
{
	double xMin = bounds[0];
	double xMax = bounds[1];
	double yMin = bounds[2];
	double yMax = bounds[3];
	double zMin = bounds[4];
	double zMax = bounds[5];

	switch (plane)
	{
	case 0:
		//Plane 1
		*planeOrigin = iAVec3d(xMin, yMin, zMax);
		*planeP1 = iAVec3d(xMax, yMin, zMax); 
		*planeP2 = iAVec3d(xMin, yMax, zMax);
		break;
	case 1:
		//Plane 2
		*planeOrigin = iAVec3d(xMax, yMin, zMax);
		*planeP1 = iAVec3d(xMax, yMin, zMin);
		*planeP2 = iAVec3d(xMax, yMax, zMax);
		break;
	case 2:
		//Plane 3
		*planeOrigin = iAVec3d(xMin, yMin, zMin);
		*planeP1 = iAVec3d(xMax, yMin, zMin);
		*planeP2 = iAVec3d(xMin, yMax, zMin);
		break;
	case 3:
		//Plane 4
		*planeOrigin = iAVec3d(xMin, yMin, zMax);
		*planeP1 = iAVec3d(xMin, yMin, zMin);
		*planeP2 = iAVec3d(xMin, yMax, zMax);
		break;
	case 4:
		//Plane 5
		*planeOrigin = iAVec3d(xMax, yMin, zMax);
		*planeP1 = iAVec3d(xMax, yMin, zMin);
		*planeP2 = iAVec3d(xMin, yMin, zMax);
		break;
	case 5:
		//Plane 6
		*planeOrigin = iAVec3d(xMax, yMax, zMax);
		*planeP1 = iAVec3d(xMax, yMax, zMin);
		*planeP2 = iAVec3d(xMin, yMax, zMax);
		break;
	}

}
