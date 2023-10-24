// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAVRObjectCoverage.h"

#include <iAColoredPolyObjectVis.h>

#include <iALog.h>

#include <vtkPointData.h>
#include <vtkMath.h>

iAVRObjectCoverage::iAVRObjectCoverage(vtkTable* objectTable, ColMapP mapping, iACsvConfig csvConfig, std::vector<iAVROctree*>* octrees, iAVRObjectModel* volume)
	: m_objectTable(objectTable), m_mapping(mapping), m_csvConfig(csvConfig), m_octrees(octrees), m_volume(volume)
{
	m_objectCoverage = new std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>();
	initialize();
}

//! Computes the coverage of objects in every octree level and region. 
//! The coverage, depending on its shape, is calculated from the ratio of the length of each part in the respective region to the objects total size
void iAVRObjectCoverage::calculateObjectCoverage()
{
	switch (m_csvConfig.visType)
	{
	case iAObjectVisType::Line:
	case iAObjectVisType::Cylinder:
		calculateCurvedLineCoverage();
		break;
	case iAObjectVisType::Ellipsoid:
		calculateEllipsoidCoverage();
		break;
	default:
		LOG(lvlError, QString("Coverage calculation for object not found"));
		break;
	}

	LOG(lvlInfo, QString("Object coverage calculated"));
	//printObjectCoverage();
}

std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* iAVRObjectCoverage::getObjectCoverage()
{
	return m_objectCoverage;
}

//! Returns the iD (row of csv) of the fiber corresponding to the polyObject ID
//! Returns -1 if point is not found in csv
vtkIdType iAVRObjectCoverage::getObjectiD(vtkIdType polyPoint)
{
	auto arr = m_volume->getPolyObject()->polyData()->GetPointData()->GetAbstractArray("");

	if (arr != nullptr)
	{
		vtkVariant v = arr->GetVariantValue(polyPoint);
		vtkIdType objID = v.ToInt() +1;	//fiber index starting at 1 not at 0
		return objID;
	}
	else
	{
		LOG(lvlDebug, QString("Point ID not found in csv"));
		return -1;
	}
}

void iAVRObjectCoverage::initialize()
{
	//Initialize new Vectors
	for (size_t level = 0; level < m_octrees->size(); level++)
	{
		//Initialize the region vec for every level
		m_objectCoverage->push_back(std::vector<std::unordered_map<vtkIdType, double>*>());

		for (vtkIdType i = 0; i < m_octrees->at(level)->getNumberOfLeafeNodes(); i++)
		{
			//Initialize a vec of Maps for every region
			m_objectCoverage->at(level).push_back(new std::unordered_map<vtkIdType, double>());
		}
	}
}

//! Computes the coverage of line objects for every octree level and region.
//! Calculates possible intersection between start- and endpoint with the bounding box of the octree regions.
void iAVRObjectCoverage::calculateLineCoverage()
{
	// For every fiber in csv table
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		double startPos[3]{}, endPos[3]{};
		for (int k = 0; k < 3; ++k)
		{
			startPos[k] = m_objectTable->GetValue(row, m_mapping->value(iACsvConfig::StartX + k)).ToFloat();
			endPos[k] = m_objectTable->GetValue(row, m_mapping->value(iACsvConfig::EndX + k)).ToFloat();
		}

		//For every Octree Level
		//for (int level = OCTREE_MIN_LEVEL; level <= 1; level++)
		for (size_t level = 0; level < m_octrees->size(); level++)
		{
			//Skip intersection test on lowest Octree level
			if (level == 0)
			{
				//Every fiber is 100% in the one region
				m_objectCoverage->at(0).at(0)->insert(std::make_pair(row, 1.0));
			}
			else
			{
				double lineLength = m_objectTable->GetValue(row, m_mapping->value(iACsvConfig::Length)).ToFloat();

				getOctreeFiberCoverage(startPos, endPos, level, row, lineLength);

			}
		}
	}

	for (size_t level = 1; level < m_octrees->size(); level++)
	{
		m_octrees->at(level)->getRegionsInLineOfRay();
	}
}

//! Computes the coverage of line objects (including optional additional sample points) for every octree level and region.
//! Calculates possible intersection between parts of the line (formed by two points) with the bounding box of the octree regions.
void iAVRObjectCoverage::calculateCurvedLineCoverage()
{
	// For every fiber in csv table
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		//For every Octree Level
		for (size_t level = 0; level < m_octrees->size(); level++)
		{
			//Skip intersection test on lowest Octree level
			if (level == 0)
			{
				//Every fiber is 100% in the one region
				m_objectCoverage->at(0).at(0)->insert(std::make_pair(row, 1.0));
			}
			else
			{
				// Only use Curved Length where curved is 1 and for the remaining Straight length
				double lineLength = 0;

				if(m_objectTable->GetValue(row, 13).ToInt())
				{
					lineLength = m_objectTable->GetValue(row, 8).ToFloat(); // Curved Length
				}
				else
				{
					lineLength = m_objectTable->GetValue(row, m_mapping->value(iACsvConfig::Length)).ToFloat();
				}

				auto endPointID = m_volume->getPolyObject()->objectStartPointIdx(row) + m_volume->getPolyObject()->objectPointCount(row);

				for (auto pointID = m_volume->getPolyObject()->objectStartPointIdx(row); pointID < endPointID - 1; ++pointID)
				{
					double startPos[3]{}, endPos[3]{};
					m_volume->getPolyObject()->polyData()->GetPoint(pointID, startPos);
					m_volume->getPolyObject()->polyData()->GetPoint(pointID+1, endPos);

					getOctreeFiberCoverage(startPos, endPos, level, row, lineLength);
				}
			}
		}
	}

	for (size_t level = 1; level < m_octrees->size(); level++)
	{
		m_octrees->at(level)->getRegionsInLineOfRay();
	}
}

//! Computes the coverage of ellipsoid objects for every octree level and region.
//! Calculates the possible intersection points from the center of the ellipse to the 6 possible, axes parallel (-x,+x,-y,+y,-z,+z),
//!  points within radius distance.
void iAVRObjectCoverage::calculateEllipsoidCoverage()
{
	// For every pore in csv table
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{

		double center[3]{}, radius[3]{};
		for (vtkIdType k = 0; k < 3; ++k)
		{
			radius[k] = m_objectTable->GetValue(row, 13 + k).ToFloat();
			center[k] = m_objectTable->GetValue(row, 18 + k).ToFloat();
		}

		//For every Octree Level
		for (size_t level = 0; level < m_octrees->size(); level++)
		{
			//Skip intersection test on lowest Octree level
			if (level == 0)
			{
				//Every fiber is 100% in the one region
				m_objectCoverage->at(0).at(0)->insert(std::make_pair(row, 1.0));
			}
			else
			{
				
				double xMinus[3] = { center[0] - radius[0], center[1], center[2] };
				double xPlus[3] = { center[0] + radius[0], center[1], center[2] };
				double yMinus[3] = { center[0], center[1] - radius[1], center[2] };
				double yPlus[3] = { center[0], center[1] + radius[1], center[2] };
				double zMinus[3] = { center[0], center[1], center[2] - radius[2] };
				double zPlus[3] = { center[0], center[1], center[2] + radius[2] };

				double xSize = calculateLineCoverageRatio(xMinus, xPlus, 1.0);
				double ySize = calculateLineCoverageRatio(yMinus, yPlus, 1.0);;
				double zSize = calculateLineCoverageRatio(zMinus, zPlus, 1.0);;
				double ellipseSize = xSize + ySize + zSize;

				//Only if at least one radius is > 0
				if (ellipseSize != 0)
				{
					getOctreeFiberCoverage(center, xMinus, level, row, ellipseSize);
					getOctreeFiberCoverage(center, xPlus, level, row, ellipseSize);
					getOctreeFiberCoverage(center, yMinus, level, row, ellipseSize);
					getOctreeFiberCoverage(center, yPlus, level, row, ellipseSize);
					getOctreeFiberCoverage(center, zMinus, level, row, ellipseSize);
					getOctreeFiberCoverage(center, zPlus, level, row, ellipseSize);
				}
				else
				{
					// ellipse is a single point
					double eps = 0.00001;
					getOctreeFiberCoverage(center, xMinus, level, row, eps);
				}


			}
		}
	}

	for (size_t level = 1; level < m_octrees->size(); level++)
	{
		m_octrees->at(level)->getRegionsInLineOfRay();
	}
}

//! The calculated intersection points are returned as vtkPoints
vtkSmartPointer<vtkPoints> iAVRObjectCoverage::getOctreeFiberCoverage(double startPoint[3], double endPoint[3], vtkIdType octreeLevel, vtkIdType fiber, double fiberLength)
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

		//The ray between start to endpoint can only intersect 2 times with one octree region bounding box
		while (pointsInRegion < 2)
		{
			double intersectionPoint[3] = { -1, -1, -1 };

			if (startPointInsideRegion == region && endPointInsideRegion == region)
			{
				double coverage = calculateLineCoverageRatio(startPoint, endPoint, fiberLength);
				pointsInRegion = 2;
				storeObjectCoverage(octreeLevel, region, fiber, coverage);
				return additionalIntersectionPoints; // whole fiber is in one region -> no intersection possible
			}
			else if (startPointInsideRegion == region)
			{
				if (checkIntersectionWithBox(startPoint, endPoint, bounds, intersectionPoint))
				{
					additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
					double coverage = calculateLineCoverageRatio(startPoint, intersectionPoint, fiberLength);
					pointsInRegion = 2;
					storeObjectCoverage(octreeLevel, region, fiber, coverage);
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
					double coverage = calculateLineCoverageRatio(intersectionPoint, endPoint, fiberLength);
					pointsInRegion = 2;
					storeObjectCoverage(octreeLevel, region, fiber, coverage);
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
						double coverage = calculateLineCoverageRatio(lastIntersection, intersectionPoint, fiberLength);
						pointsInRegion = 2;
						storeObjectCoverage(octreeLevel, region, fiber, coverage);
						break;
					}
					else if (checkIntersectionWithBox(lastIntersection, startPoint, bounds, intersectionPoint))
					{
						additionalIntersectionPoints->InsertNextPoint(lastIntersection);
						additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
						double coverage = calculateLineCoverageRatio(lastIntersection, intersectionPoint, fiberLength);
						pointsInRegion = 2;
						storeObjectCoverage(octreeLevel, region, fiber, coverage);
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

//! Calculates the interesection of a line (start- to endpoint) with the six planes of a bounding box
bool iAVRObjectCoverage::checkIntersectionWithBox(double startPoint[3], double endPoint[3], double bounds[6], double intersection[3])
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

//! Calculates the ratio between the startPoint to endPoint length versus the overall line length
double iAVRObjectCoverage::calculateLineCoverageRatio(double startPoint[3], double endPoint[3], double lineLength)
{
	double eps = 0.00001;

	double vectorBetweenStartAndEnd[3]{};
	for (int i = 0; i < 3; i++)
	{
		vectorBetweenStartAndEnd[i] = (endPoint[i] - startPoint[i]);
	}

	// If startPoint == endPoint -> set to eps
	if (vectorBetweenStartAndEnd[0] == 0 && vectorBetweenStartAndEnd[1] == 0 && vectorBetweenStartAndEnd[2] == 0)
	{
		vectorBetweenStartAndEnd[0] = eps;
		vectorBetweenStartAndEnd[1] = eps;
		vectorBetweenStartAndEnd[2] = eps;
	}

	double coverage = sqrt(pow(vectorBetweenStartAndEnd[0], 2) + pow(vectorBetweenStartAndEnd[1], 2) + pow(vectorBetweenStartAndEnd[2], 2));
	double ratio = std::round((coverage / lineLength)*10000.0)/ 10000.0; //round to 4 decimal places

	return ratio;
}

//! Stores the objects coverage into the map
//! If the object has already a coverage in that specific region it gets summed up
void iAVRObjectCoverage::storeObjectCoverage(vtkIdType octreeLevel, vtkIdType region, vtkIdType fiber, double coverage)
{
	auto it = m_objectCoverage->at(octreeLevel).at(region)->find(fiber);
	if(it != m_objectCoverage->at(octreeLevel).at(region)->end())
	{
		it->second = it->second + coverage; // add additional coverage of this object in this region
	}
	else
	{
		m_objectCoverage->at(octreeLevel).at(region)->insert(std::make_pair(fiber, coverage)); // first coverage
	}

	//TODO CHECK if <1 ?
}

//! Checks if two pos arrays are the same
bool iAVRObjectCoverage::checkEqualArrays(float pos1[3], float pos2[3])
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

//! Checks if two pos arrays are the same
bool iAVRObjectCoverage::checkEqualArrays(double pos1[3], double pos2[3])
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

//! Creates the three points needed to define a plane of a bounding box
void iAVRObjectCoverage::createPlanePoint(int plane, double bounds[6], iAVec3d* planeOrigin, iAVec3d* planeP1, iAVec3d* planeP2)
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

//! DebugOutput for the object coverage
void iAVRObjectCoverage::printObjectCoverage()
{
	QString output;
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		output.append("###################### \n");
		output.append(QString("Object %1: \n").arg(row));
		for (size_t level = 0; level < m_octrees->size(); level++)
		{
			output.append(QString(" Octree Level %1: \n").arg(level));

			for (size_t region = 0; region < m_objectCoverage->at(level).size(); region++)
			{
				auto it = m_objectCoverage->at(level).at(region)->find(row);

				if (it != m_objectCoverage->at(level).at(region)->end())
				{
					output.append(QString("  > Region %1 -- %2 \n").arg(region).arg(it->second));
				}
			}
			
		}
	}
	LOG(lvlImportant, output);
}
