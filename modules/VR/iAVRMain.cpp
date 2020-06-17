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
#include "iAVRMain.h"

#include <iAConsole.h>
#include "iAVRInteractor.h"
#include "iA3DCylinderObjectVis.h"
#include "iAVR3DObjectVis.h"
#include "iAVROctree.h"
#include "iAVRInteractorStyle.h"

#include "vtkRenderer.h"
#include "vtkIdList.h"
#include "vtkProperty.h"
#include "vtkActor.h"
#include "vtkPropCollection.h"
#include "vtkPointData.h"
#include "vtkAbstractPropPicker.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkPolyDataMapper.h"
#include <vtkPlaneSource.h>
#include "vtkIntersectionPolyDataFilter.h"
#include "vtkLineSource.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkScalarBarActor.h"

#include <QColor>

//Octree Max Level
#define OCTREE_MAX_LEVEL 3
#define OCTREE_MIN_LEVEL 1
#define OCTREE_POINTS_PER_REGION 1
#define OCTREE_COLOR QColor(126, 0, 223, 255)

//Offsets for the hovering Effect of the Model in Miniature
#define xOffset -0.2
#define yOffset 0.1
#define zOffset -0.2

iAVRMain::iAVRMain(iAVREnvironment* vrEnv, iAVRInteractorStyle* style, vtkTable* objectTable, iACsvIO io): m_vrEnv(vrEnv),
	m_style(style),	m_objectTable(objectTable),	m_io(io)
{
	m_cylinderVis = new iA3DCylinderObjectVis(m_vrEnv->renderer(), m_objectTable, m_io.getOutputMapping(), QColor(140,140,140,255), std::map<size_t, std::vector<iAVec3f> >());
	//m_cylinderVis->setShowLines(true);
	//m_cylinderVis->getActor()->GetProperty()->SetLineWidth(4);

	//Define Octree
	currentOctreeLevel = 0;
	m_octrees = new std::vector<iAVROctree*>();
	generateOctrees(OCTREE_MAX_LEVEL, OCTREE_POINTS_PER_REGION, m_cylinderVis->getPolyData());
	//m_octree->calculateOctree(OCTREE_MAX_LEVEL, OCTREE_POINTS_PER_REGION);

	// For true TranslucentGeometry
	//https://vtk.org/Wiki/VTK/Examples/Cxx/Visualization/CorrectlyRenderTranslucentGeometry#CorrectlyRenderTranslucentGeometry.cxx
	//m_vrEnv->renderer()->SetUseDepthPeeling(true);
	//m_vrEnv->renderer()->SetMaximumNumberOfPeels(2);

	//Initialize Metrics class
	fiberMetrics = new iAVRMetrics(m_objectTable, m_io, m_octrees);

	//map for the coverage of each fiber (Initialize with MaxLeafNodes in Max Level)
	m_fiberCoverage = new std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>();

	//Thread
	m_iDMappingThread = std::thread(&iAVRMain::mapAllPointiDs, this);
	//m_iDMappingThread = std::thread(&iAVRMain::mapAllPointiDsAndCalculateFiberCoverage, this);
	//mapAllPointiDsAndCalculateFiberCoverage();
	//mapAllPointiDs();
	//m_iDMappingThreadRunning = false;

	//Add InteractorStyle
	m_style->setVRMain(this);
	m_vrEnv->interactor()->SetInteractorStyle(m_style);

	//Add Input Mapping
	//Press, Touch
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::Volume, iAVROperations::PickSingleFiber);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::MiniatureModel, iAVROperations::PickMiMRegion);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::Volume, iAVROperations::PickFibersinRegion);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::TrackPad, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeOctreeLevel);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::ResetSelection);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::ResetSelection);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::SpawnModelInMiniature);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::MiniatureModel, iAVROperations::SpawnModelInMiniature);
	//Release, Untouch

	//Create Cube
	m_objectVis = new iAVR3DObjectVis(m_vrEnv->renderer());
	m_objectVis->setOctree(m_octrees->at(OCTREE_MIN_LEVEL));

	m_cylinderVis->show();	

	//Add Actors
	addPropToOptionID(vtkProp3D::SafeDownCast(m_objectVis->getActor()), iAVRInteractionOptions::MiniatureModel);
	addPropToOptionID(vtkProp3D::SafeDownCast(m_cylinderVis->getActor()), iAVRInteractionOptions::Volume);
	for(int i = 0; i < m_octrees->size(); i++)
	{
		addPropToOptionID(vtkProp3D::SafeDownCast(m_octrees->at(i)->getActor()), iAVRInteractionOptions::Volume); //Octree counts as Volume
	}
	
}

//! Defines the action executed for specific controller inputs
void iAVRMain::startInteraction(vtkEventDataDevice3D* device, double eventPosition[3], double eventOrientation[4], vtkProp3D* pickedProp)
{
	int deviceID = static_cast<int>(device->GetDevice()); // Device
	int inputID = static_cast<int>(device->GetInput());  // Input Method
	int actioniD = static_cast<int>(device->GetAction());     // Action of Input Method
	int optionID = getOptionForObject(pickedProp);

	// Active Input Saves the current applied operation in case Multiinput is requires
	std::vector<int>* activeInput = m_style->getActiveInput();

	inputScheme* scheme = m_style->getInputScheme();
	int operation = scheme->at(deviceID).at(inputID).at(actioniD).at(optionID);

	switch (iAVROperations(operation))
	{
	case iAVROperations::Unknown:
		DEBUG_LOG(QString("Unknown Operation!"));
		break;
	case iAVROperations::None:
		//activeInput->at(deviceID) = operation; // For Multitouch
		break;
	case iAVROperations::SpawnModelInMiniature:
		this->spawnModelInMiniature(eventPosition, modelInMiniatureActive);
		break;
	case iAVROperations::PickSingleFiber:
		this->pickSingleFiber(eventPosition);
		break;
	case iAVROperations::PickFibersinRegion:
		this->pickFibersinRegion(eventPosition);
		break;
	case iAVROperations::PickMiMRegion:
		this->pickMimRegion(eventPosition, eventOrientation);
		break;
	case iAVROperations::ChangeOctreeLevel:
		this->changeOctreeLevel();
		break;
	case iAVROperations::ResetSelection:
		this->resetSelection();
		break;
	}

	//DEBUG_LOG(QString("[START] active Input rc = %1, lc = %2").arg(activeInput->at(1)).arg(activeInput->at(2)));
	//Update Changes //ToDO: Required? Or is render Selection enough?
	m_vrEnv->update();
}

void iAVRMain::endInteraction(vtkEventDataDevice3D* device, double eventPosition[3], double eventOrientation[4], vtkProp3D* pickedProp)
{
	int deviceID = static_cast<int>(device->GetDevice()); // Device
	int inputID = static_cast<int>(device->GetInput());  // Input Method
	int actioniD = static_cast<int>(device->GetAction());     // Action of Input Method
	int optionID = getOptionForObject(pickedProp);

	// Active Input Saves the current applied operation in case Multiinput is requires
	std::vector<int>* activeInput = m_style->getActiveInput();

	inputScheme* scheme = m_style->getInputScheme();
	int operation = scheme->at(deviceID).at(inputID).at(actioniD).at(optionID);

	switch (iAVROperations(operation))
	{
	case iAVROperations::Unknown:
		DEBUG_LOG(QString("Unknown Operation!"));
		break;
	case iAVROperations::None:
		//activeInput->at(deviceID) = static_cast<int>(iAVROperations::None); // For Multitouch
		break;
	}
	//DEBUG_LOG(QString("[END] active Input rc = %1, lc = %2").arg(activeInput->at(1)).arg(activeInput->at(2)));
	//Update Changes //ToDO: Required? Or is render Selection enough?
	m_vrEnv->update();
}

void iAVRMain::onMove(vtkEventDataDevice3D * device, double movePosition[3], double eventOrientation[4], vtkProp3D * pickedProp)
{
	int deviceID = static_cast<int>(device->GetDevice());
	cPos[deviceID][0] = movePosition[0];
	cPos[deviceID][1] = movePosition[1];
	cPos[deviceID][2] = movePosition[2];

	//The Model in Minature follows the left controller
	if(modelInMiniatureActive && deviceID == static_cast<int>(vtkEventDataDevice::LeftController))
	{
		double sideDistance = *(m_objectVis->getActor()->GetYRange());
		double hoveringOffset[3] = { sideDistance *xOffset, sideDistance * yOffset, sideDistance * zOffset };

		m_objectVis->setPos(cPos[deviceID][0] + hoveringOffset[0], cPos[deviceID][1] + hoveringOffset[1], cPos[deviceID][2] + hoveringOffset[2]);
		//Set Orientation!!
	}
}

//! Computes which polyObject ID (points) belongs to which Object ID in the csv file of the volume
//! Gets only called internally from thread to store the mapping
void iAVRMain::mapAllPointiDs()
{
	// For every fiber in csv table
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		double startPos[3], endPos[3];
		for (int k = 0; k < 3; ++k)
		{
			startPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::StartX + k)).ToFloat();
			endPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::EndX + k)).ToFloat();
		}
		// Insert polyObject ID of Start Point
		m_pointIDToCsvIndex.insert(std::make_pair(m_cylinderVis->getPolyData()->FindPoint(startPos), row));
		// Insert polyObject ID of End Point
		m_pointIDToCsvIndex.insert(std::make_pair(m_cylinderVis->getPolyData()->FindPoint(endPos), row));
	}
	DEBUG_LOG(QString("Volume Data loaded"));

	//Calculate Fibers in Region
	for (int i = 0; i < m_octrees->size(); i++)
	{
		m_fiberCoverage->push_back(*m_octrees->at(i)->getfibersInRegionMapping(&m_pointIDToCsvIndex));	
	}
	DEBUG_LOG(QString("Fibers in Region for every octree calculated"));

	m_iDMappingThreadRunning = false; //Thread ended
}

//! Computes which polyObject ID (points) belongs to which Object ID in the csv file of the volume for every Octree Level
//! Calculates Intersection between points in different Octree regions in all levels. Calls a emthod to compute fiber coverage.
//! Gets only called internally from thread to store the mapping
void iAVRMain::mapAllPointiDsAndCalculateFiberCoverage()	//TODO Replaces the mapAllPointiDs Method (+thread)
{
	//m_extendedCylinderVisData = vtkSmartPointer<vtkPolyData>::New();
	//m_extendedCylinderVisData = m_cylinderVis->getPolyData();
	//m_extendedCylinderVisData->DeepCopy(m_cylinderVis->getPolyData()); // Use copy of volume data

	// For every fiber in csv table
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{

		double startPos[3], endPos[3];
		for (int k = 0; k < 3; ++k)
		{
			startPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::StartX + k)).ToFloat();
			endPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::EndX + k)).ToFloat();
		}

		// Insert polyObject ID of Start Point
		m_pointIDToCsvIndex.insert(std::make_pair(m_cylinderVis->getPolyData()->FindPoint(startPos), row));
		// Insert polyObject ID of End Point
		m_pointIDToCsvIndex.insert(std::make_pair(m_cylinderVis->getPolyData()->FindPoint(endPos), row));

		//For every Octree Level
		//for(int level = OCTREE_MIN_LEVEL; level <= m_octrees->size(); level++)
		for (int level = OCTREE_MIN_LEVEL; level <= 1; level++)
		{

			DEBUG_LOG(QString("## Level Number: %1 ##").arg(level));
			//DEBUG_LOG(QString("startPos: %1 | %2 | %3").arg(startPos[0]).arg(startPos[1]).arg(startPos[2]));
			//DEBUG_LOG(QString("endPos: %1 | %2 | %3").arg(endPos[0]).arg(endPos[1]).arg(endPos[2]));

			double fiberLength = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::Length)).ToFloat();
			std::vector<double>* coverageInRegion = new std::vector<double>();
			vtkSmartPointer<vtkPoints> intersectionPoints = vtkSmartPointer<vtkPoints>::New();

			intersectionPoints = getOctreeFiberCoverage(startPos, endPos, level, fiberLength, coverageInRegion);

			if (intersectionPoints == nullptr)
			{
				DEBUG_LOG(QString("!! vtkPoints is null..."));
			}
			else
			{
				DEBUG_LOG(QString("#################################################"));
				DEBUG_LOG(QString("# |%1| Intersections for fiber |%2| at level %3 #").arg(intersectionPoints->GetNumberOfPoints()).arg(row).arg(level));
				DEBUG_LOG(QString("#################################################\n"));
			}

			//m_fiberCoverage->at(level).at(region)->insert(std::make_pair(row, coverage));

			//ADD the new Points to the Polydata and the Mapping
			//m_extendedCylinderVisData->GetPoints()->InsertNextPoint(intersectionPoint);
			// bzw. m_extendedCylinderVisData->SetPoints(intersectionPoints)
			//m_extendedCylinderVisData->GetPoints()->Modified();
			//m_pointIDToCsvIndex.insert(std::make_pair(m_extendedCylinderVisData->FindPoint(intersectionPoint), row));

			
		}
	}
	
	DEBUG_LOG(QString("Volume Data loaded with Method 2"));
	m_iDMappingThreadRunning = false; //Thread ended
}

//!
//! The calculated intersection points are returned as vtkPoints
vtkSmartPointer<vtkPoints> iAVRMain::getOctreeFiberCoverage(double startPoint[3], double endPoint[3], int octreeLevel, double fiberLength, std::vector<double>* coverageInRegion)
{
	vtkSmartPointer<vtkPoints> additionalIntersectionPoints = vtkSmartPointer<vtkPoints>::New();

	//m_octree->calculateOctree(octreeLevel, OCTREE_POINTS_PER_REGION);
	int leafNodes = m_octrees->at(octreeLevel)->getNumberOfLeafeNodes();
	int startPointInsideRegion = m_octrees->at(octreeLevel)->getOctree()->GetRegionContainingPoint(startPoint[0], startPoint[1], startPoint[2]);
	int endPointInsideRegion = m_octrees->at(octreeLevel)->getOctree()->GetRegionContainingPoint(endPoint[0], endPoint[1], endPoint[2]);
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

	coverageInRegion = new std::vector<double>(0.0, leafNodes);

	for (int region = 0; region < leafNodes; region++)
	{
		double lastIntersection[3] = { -1, -1, -1 };
		int pointsInRegion = 0;
		double bounds[6];
		std::vector<vtkSmartPointer<vtkPlaneSource>>* planes = new std::vector<vtkSmartPointer<vtkPlaneSource>>();
		m_octrees->at(octreeLevel)->createOctreeBoundingBoxPlanes(region, planes);
		m_octrees->at(octreeLevel)->getOctree()->GetRegionBounds(region, bounds);

		DEBUG_LOG(QString("\n## Region Number: %1 ##\n").arg(region));
		DEBUG_LOG(QString("StartPoint in Region |%1| and EndPoint in Region |%2|").arg(startPointInsideRegion).arg(endPointInsideRegion));
		std::vector<double*>* point = new std::vector<double*>();
		
		//The ray between start to endpoint can only intersect 2 times with a octree region bounding box
		while (pointsInRegion < 2)
		{
			double intersectionPoint[3] = { -1, -1, -1 };

			if (startPointInsideRegion == region && endPointInsideRegion == region)
			{
				double coverage = calculateFiberCoverage(startPoint, endPoint, fiberLength);
				pointsInRegion = 2;
				coverageInRegion->push_back(coverage);
				break;
			}
			else if (startPointInsideRegion == region)
			{
				DEBUG_LOG(QString("StartPoint is in Region"));
				if (checkIntersectionWithBox(startPoint, endPoint, planes, bounds, intersectionPoint))
				{
					additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
					double coverage = calculateFiberCoverage(startPoint, intersectionPoint, fiberLength);
					pointsInRegion = 2;
					coverageInRegion->push_back(coverage);
					DEBUG_LOG(QString(">> PLANE HIT! at %1 | %2 | %3 <").arg(intersectionPoint[0]).arg(intersectionPoint[1]).arg(intersectionPoint[2]));
					point->push_back(intersectionPoint);
					break;
				}
				else
				{
					// startpoint and Intersection point are the same position (lie on same boundary but are in different regions for the octree)
					DEBUG_LOG(QString("Worst case (start) - break"));
					break;
				}
			}
			else if (endPointInsideRegion == region)
			{
				DEBUG_LOG(QString("Endpoint is in Region"));
				if (checkIntersectionWithBox(endPoint, startPoint, planes, bounds, intersectionPoint))
				{
					additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
					double coverage = calculateFiberCoverage(intersectionPoint, endPoint, fiberLength);
					pointsInRegion = 2;
					coverageInRegion->push_back(coverage);
					DEBUG_LOG(QString(">> PLANE HIT! at %1 | %2 | %3 <").arg(intersectionPoint[0]).arg(intersectionPoint[1]).arg(intersectionPoint[2]));
					point->push_back(intersectionPoint);
					break;
				}
				else
				{
					// endPoint and Intersection point are the same position (lie on same boundary but are in different regions for the octree)
					DEBUG_LOG(QString("Worst case (end) - break"));
					break;
				}
			}
			else
			{
				DEBUG_LOG(QString("Looking for two points!!!!"));
				//When a second point was found...
				if (pointsInRegion > 0)
				{
					if (checkIntersectionWithBox(lastIntersection, endPoint, planes, bounds, intersectionPoint))
					{
						additionalIntersectionPoints->InsertNextPoint(lastIntersection);
						additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
						double coverage = calculateFiberCoverage(lastIntersection, intersectionPoint, fiberLength);
						pointsInRegion = 2;
						coverageInRegion->push_back(coverage);
						DEBUG_LOG(QString(">> PLANE HIT 1! at %1 | %2 | %3 <").arg(lastIntersection[0]).arg(lastIntersection[1]).arg(lastIntersection[2]));
						DEBUG_LOG(QString(">> PLANE HIT 2! at %1 | %2 | %3 <").arg(intersectionPoint[0]).arg(intersectionPoint[1]).arg(intersectionPoint[2]));
						point->push_back(lastIntersection);
						point->push_back(intersectionPoint);
						break;
					}
					else
					{
						//Corner Point with no second point in region
						DEBUG_LOG(QString("Worst case (second point) - break"));
						break;
					}

				}
				else // look if there is a intersection with a region outside of the startPoint/endPoint region
				{
					
					if (checkIntersectionWithBox(startPoint, endPoint, planes, bounds, intersectionPoint))
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
		if (region % 3 == 0) drawPoint(point, QColor(255,0,0));
		if (region % 3 == 1) drawPoint(point, QColor(0, 255, 0));
		if (region % 3 == 2) drawPoint(point, QColor(0, 0, 255));
	}

	return additionalIntersectionPoints;
}

//! Looks in the vtkTable for the given position (could be start or end position)
//! Returns the row iD of the found entrance in the table
//! Converts double pos to float for use in vtk! Returns -1 if point is not found in csv
vtkIdType iAVRMain::mapSinglePointiD(vtkIdType polyPoint)
{
	double pos[3];
	m_octrees->at(currentOctreeLevel)->getOctree()->GetDataSet()->GetPoint(polyPoint, pos);

	float posf[3];
	// Convert to float
	for (int i = 0; i < 3; ++i)
	{
		posf[i] = (float)(pos[i]);
	}

	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{

		float startPos[3], endPos[3];
		for (int i = 0; i < 3; ++i)
		{
			startPos[i] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::StartX + i)).ToFloat();
			endPos[i] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::EndX + i)).ToFloat();
		}

		if (checkEqualArrays(posf, startPos) || checkEqualArrays(posf, endPos))
		{
			return row;
		}

	}

	return -1;
}

//! Checks if two pos arrays are the same
bool iAVRMain::checkEqualArrays(float pos1[3], float pos2[3])
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

void iAVRMain::setInputScheme(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, iAVRInteractionOptions options, iAVROperations operation)
{
	inputScheme* scheme = m_style->getInputScheme();

	if(options == iAVRInteractionOptions::Anywhere) //Apply Operation for every Interaction Option
	{
		for(int i=0; i < static_cast<int>(iAVRInteractionOptions::NumberOfInteractionOptions); i++)
		{
			scheme->at(static_cast<int>(device)).at(static_cast<int>(input)).at(static_cast<int>(action)).at(static_cast<int>(i)) = static_cast<int>(operation);
		}
	}
	else
	{
		scheme->at(static_cast<int>(device)).at(static_cast<int>(input)).at(static_cast<int>(action)).at(static_cast<int>(options)) = static_cast<int>(operation);
	}
}

//! Returns which InteractionOption is for the currently picked Object available 
int iAVRMain::getOptionForObject(vtkProp3D* pickedProp)
{
	if (pickedProp == nullptr)
	{
		return static_cast<int>(iAVRInteractionOptions::NoObject);
	}
	else
	{
		if(!m_ActorToOptionID.count(pickedProp))
		{
			DEBUG_LOG(QString("Picked Object Unknown!"));
			return -1;
		}
		return m_ActorToOptionID.at(pickedProp);
	}
}

void iAVRMain::addPropToOptionID(vtkProp3D* prop, iAVRInteractionOptions iD)
{
	m_ActorToOptionID.insert(std::make_pair(prop, static_cast<int>(iD)));
}

bool iAVRMain::checkIntersectionWithBox(double startPoint[3], double endPoint[3], std::vector<vtkSmartPointer<vtkPlaneSource>>* planes, double bounds[6], double intersection[3])
{
	double eps = 0.00001;

	iAVec3d p0 = iAVec3d(startPoint);
	iAVec3d p1 = iAVec3d(endPoint);
	iAVec3d ray = p1 - p0;
	iAVec3d ray_normalized = ray;
	ray_normalized.normalize();

	//if (ray.length() < eps) return false; // startPoint == endPoint

	for (int i = 0; i < planes->size(); i++)
	{
		iAVec3d normal = iAVec3d(planes->at(i)->GetNormal());
		//DEBUG_LOG(QString("normal = %1 | %2 | %3 ").arg(normal[0]).arg(normal[1]).arg(normal[2]));
		normal.normalize();
		iAVec3d pointOnPlaneOrigin = iAVec3d(planes->at(i)->GetOrigin()); 
		iAVec3d pointOnPlane1 = iAVec3d(planes->at(i)->GetPoint1());
		iAVec3d pointOnPlane2 = iAVec3d(planes->at(i)->GetPoint2());
		iAVec3d planeCenter = iAVec3d(planes->at(i)->GetCenter());

		//DEBUG_LOG(QString("origin %1 | %2 | normal %3 |  %4 \n").arg(pointOnPlaneOrigin[0]).arg(pointOnPlaneOrigin[1]).arg(normal[0]).arg(normal[1]));
		//DEBUG_LOG(QString("startPos %1 | %2 | %3 \n").arg(startPos[0]).arg(startPos[1]).arg(startPos[2]));
		//DEBUG_LOG(QString("endPos %1 | %2 | %3 \n").arg(endPos[0]).arg(endPos[1]).arg(endPos[2]));

		//DEBUG_LOG(QString("ray %1 | %2 | %3 \n").arg(ray[0]).arg(ray[1]).arg(ray[2]));
		//DEBUG_LOG(QString("ray_normalized %1 | %2 | %3 \n").arg(ray_normalized[0]).arg(ray_normalized[1]).arg(ray_normalized[2]));
		//vtkSmartPointer<vtkLineSource> raySource = vtkSmartPointer<vtkLineSource>::New();
		//raySource->SetPoint1(startPoint);
		//raySource->SetPoint2(endPoint);
		//raySource->Update();
		//vtkSmartPointer<vtkPolyDataMapper> rayMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		//rayMapper->SetInputConnection(raySource->GetOutputPort());
		//vtkSmartPointer<vtkActor> rayActor = vtkSmartPointer<vtkActor>::New();
		//rayActor->SetMapper(rayMapper);
		//rayActor->GetProperty()->SetRenderLinesAsTubes(true);
		//rayActor->GetProperty()->SetColor(1.0, 0.0, 0.7);
		//rayActor->GetProperty()->SetLineWidth(2);
		//m_vrEnv->renderer()->AddActor(rayActor);
	
		if (abs(dotProduct(normal, ray_normalized)) > eps) { // If false -> the line is parallel to the plane
			iAVec3d difference = planeCenter - p0;

			// Compute the t value for the directed line ray intersecting the plane
			double t = dotProduct(difference, normal) / dotProduct(normal, ray_normalized);

			// possible intersection point in infinety
			intersection[0] = p0[0] + (ray_normalized[0] * t);
			intersection[1] = p0[1] + (ray_normalized[1] * t);
			intersection[2] = p0[2] + (ray_normalized[2] * t);
			
			// Intersection point is start or end point
			//iAVec3d intersectionPos = iAVec3d(intersection);
			//if (((p0 - intersectionPos).length() < eps) || ((p1 - intersectionPos).length() < eps))
			//{
			//	continue;
			//}

			//DEBUG_LOG(QString("t = %1 ").arg(t));

			//DEBUG_LOG(QString("origin = %1 | %2 | %3 ").arg(pointOnPlaneOrigin[0]).arg(pointOnPlaneOrigin[1]).arg(pointOnPlaneOrigin[2]));
			//DEBUG_LOG(QString("point 1 = %1 | %2 | %3 ").arg(pointOnPlane1[0]).arg(pointOnPlane1[1]).arg(pointOnPlane1[2]));
			//DEBUG_LOG(QString("point 2 = %1 | %2 | %3 ").arg(pointOnPlane2[0]).arg(pointOnPlane2[1]).arg(pointOnPlane2[2]));

			//vtkSmartPointer<vtkPolyDataMapper> planeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
			//planeMapper->SetInputConnection(planes->at(i)->GetOutputPort());
			//vtkSmartPointer<vtkActor> planeActor = vtkSmartPointer<vtkActor>::New();
			//planeActor->SetMapper(planeMapper);
			//planeActor->GetProperty()->SetColor(1.0, 0.4, 0.0);
			//m_vrEnv->renderer()->AddActor(planeActor);

			//Intersection must be in the inside the finite plane
			if((t > eps) && 
				(intersection[0] <= bounds[1]) && (intersection[0] >= bounds[0]) &&
				(intersection[1] <= bounds[3]) && (intersection[1] >= bounds[2]) &&
				(intersection[2] <= bounds[5]) && (intersection[2] >= bounds[4]))
			{
	
				//DEBUG_LOG(QString(">> PLANE HIT! at %1 | %2 | %3 <").arg(intersection[0]).arg(intersection[1]).arg(intersection[2]));

				return true;
			
			}

			//DEBUG_LOG(QString(">> NO INTERSECTION FOUND at  %1 | %2 | %3").arg(intersection[0]).arg(intersection[1]).arg(intersection[2]));

		}
	}
	return false;
}

double iAVRMain::calculateFiberCoverage(double startPoint[3], double endPoint[3], double fiberLength)
{
	double vectorBetweenStartAndEnd[3];
	for(int i = 0; i < 3; i++)
	{
		vectorBetweenStartAndEnd[i] = (endPoint[i] - startPoint[i]);
	}
	double coverage = sqrt(pow(vectorBetweenStartAndEnd[0], 2) + pow(vectorBetweenStartAndEnd[1], 2) + pow(vectorBetweenStartAndEnd[2], 2));
	
	return coverage/fiberLength;
}

//! Test method inserts colored point at given Position
void iAVRMain::drawPoint(std::vector<double*>* pos, QColor color)
{
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

	for(int i=0; i<pos->size(); i++)
	{
		points->InsertNextPoint(pos->at(i));
	}

	vtkSmartPointer<vtkPolyData> pointsPolydata = vtkSmartPointer<vtkPolyData>::New();
	pointsPolydata->SetPoints(points);

	vtkSmartPointer<vtkVertexGlyphFilter> vertexGlyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
	vertexGlyphFilter->AddInputData(pointsPolydata);
	vertexGlyphFilter->Update();

	vtkSmartPointer<vtkPolyDataMapper> pointsMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	pointsMapper->SetInputConnection(vertexGlyphFilter->GetOutputPort());
	vtkSmartPointer<vtkActor> pointsActor = vtkSmartPointer<vtkActor>::New();
	pointsActor->SetMapper(pointsMapper);
	pointsActor->GetProperty()->SetPointSize(8);
	pointsActor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
	m_vrEnv->renderer()->AddActor(pointsActor);

}

//! If mapping thread has finsihed load data from map, otherwise calculate it
//! Returns -1 if point is not found in csv
vtkIdType iAVRMain::getObjectiD(vtkIdType polyPoint)
{
	if (!m_iDMappingThreadRunning)
	{
		if (m_iDMappingThread.joinable())
		{
			m_iDMappingThread.join();
		}

		if (m_pointIDToCsvIndex.find(polyPoint) != m_pointIDToCsvIndex.end())
		{
			return m_pointIDToCsvIndex.at(polyPoint);
		}
		else
		{
			DEBUG_LOG(QString("Point not found in csv"));
			return -1;
		}

	}
	else
	{
		return mapSinglePointiD(polyPoint);
	}

}

//! Generates octrees until maxLevel is reached or no more leafNodes are added through later levels
void iAVRMain::generateOctrees(int maxLevel, int maxPointsPerRegion, vtkPolyData* dataSet)
{
	int lastLeafNodeAmount = 0;
	for(int level = 0; level <= maxLevel; level++)
	{
		iAVROctree* tempOctree = new iAVROctree(m_vrEnv->renderer(), dataSet);
		tempOctree->calculateOctree(level, maxPointsPerRegion);
		// Check if leafNodes have been added, if not stop creating deeper levels
		if (tempOctree->getNumberOfLeafeNodes() <= lastLeafNodeAmount) break;
		lastLeafNodeAmount = tempOctree->getNumberOfLeafeNodes();

		m_octrees->push_back(tempOctree);
	}
	if(m_octrees->empty()) DEBUG_LOG(QString("NO OCTREE GENERATED"));
}

void iAVRMain::calculateMetrics()
{
	//Gets only called when thread is finished
	if(!m_iDMappingThreadRunning){
		fiberMetrics->setFiberCoverageData(m_fiberCoverage);
		std::vector<std::vector<double>>* rgba = fiberMetrics->getHeatmapColoring(currentOctreeLevel, 9);

		//DEBUG_LOG(QString(">In Main the 1 color is: %1 / %2 / %3 / %4").arg(rgba->at(0).at(0)).arg(rgba->at(0).at(1)).arg(rgba->at(0).at(2)).arg(rgba->at(0).at(3)));
		//DEBUG_LOG(QString(">In Main the 2 color is: %1 / %2 / %3 / %4").arg(rgba->at(1).at(0)).arg(rgba->at(1).at(1)).arg(rgba->at(1).at(2)).arg(rgba->at(1).at(3)));

		m_objectVis->applyHeatmapColoring(rgba);

		fiberMetrics->getColorBarLegend()->SetTitle("Length");
		
	}
}

//! Updates the data (Octree, Metrics,...) for the current MiM
void iAVRMain::updateModelInMiniatureData()
{
	m_objectVis->setOctree(m_octrees->at(currentOctreeLevel));
	m_objectVis->createModelInMiniature();
	calculateMetrics();
}

//! Increases the current octree level by one until max level and repeats. Recalculates the Model in Miniature Object.
void iAVRMain::changeOctreeLevel()
{
	m_octrees->at(currentOctreeLevel)->hide(); // Hide old octree
	
	currentOctreeLevel++;

	if (currentOctreeLevel >= m_octrees->size() || currentOctreeLevel < OCTREE_MIN_LEVEL)
	{		
		currentOctreeLevel = OCTREE_MIN_LEVEL;
	}

	//m_octree->calculateOctree(currentOctreeLevel, OCTREE_POINTS_PER_REGION);
	m_octrees->at(currentOctreeLevel)->generateOctreeRepresentation(currentOctreeLevel, OCTREE_COLOR);
	m_octrees->at(currentOctreeLevel)->show();
	updateModelInMiniatureData();
}

void iAVRMain::pickSingleFiber(double eventPosition[3])
{
	std::vector<size_t> selection = std::vector<size_t>();
	// Find the closest points to TestPoint
	vtkIdType iD = m_octrees->at(currentOctreeLevel)->getOctree()->FindClosestPoint(eventPosition);

	vtkIdType rowiD = getObjectiD(iD);
	selection.push_back(rowiD);

	m_cylinderVis->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
}

//! Picks all fibers in the region clicked by the user.
void iAVRMain::pickFibersinRegion(double eventPosition[3])
{
	std::vector<size_t> selection = std::vector<size_t>();

	int leafRegion = m_octrees->at(currentOctreeLevel)->getOctree()->GetRegionContainingPoint(eventPosition[0], eventPosition[1], eventPosition[2]);
	vtkIdTypeArray *points = m_octrees->at(currentOctreeLevel)->getOctree()->GetPointsInRegion(leafRegion);

	//Check if points is null!!
	if (points == nullptr) {
		DEBUG_LOG(QString("No points in the region!"));
		return;
	}

	DEBUG_LOG(QString("Region clicked: %1 which has %2 fibers").arg(leafRegion).arg(points->GetSize()));

	for (int i = 0; i < points->GetSize(); i++)
	{
		vtkIdType rowiD = getObjectiD(points->GetValue(i));
		selection.push_back(rowiD);

		double pos[3];
		m_octrees->at(currentOctreeLevel)->getOctree()->GetDataSet()->GetPoint(points->GetValue(i), pos);
		//DEBUG_LOG(QString("Fiber (%1) has Coord.: <%2 | %3 | %4>").arg(i).arg(pos[0]).arg(pos[1]).arg(pos[2]));
	}

	std::sort(selection.begin(), selection.end());
	selection.erase(std::unique(selection.begin(), selection.end()), selection.end());

	m_cylinderVis->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
	
}

//! Picks all fibers in the octree region defined by the leaf node ID.
void iAVRMain::pickFibersinRegion(int leafRegion)
{
	std::vector<size_t> selection = std::vector<size_t>();
	vtkIdTypeArray *points = m_octrees->at(currentOctreeLevel)->getOctree()->GetPointsInRegion(leafRegion);

	//Check if points is null!!
	if (points == nullptr) {
		DEBUG_LOG(QString("No points in the region!"));
		return;
	}

	for (int i = 0; i < points->GetSize(); i++)
	{
		vtkIdType rowiD = getObjectiD(points->GetValue(i));
		selection.push_back(rowiD);
	}

	std::sort(selection.begin(), selection.end());
	selection.erase(std::unique(selection.begin(), selection.end()), selection.end());

	m_cylinderVis->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);

}

void iAVRMain::pickMimRegion(double eventPosition[3], double eventOrientation[4])
{
	vtkIdType cellID = m_objectVis->getClosestCellID(eventPosition, eventOrientation);

	if(cellID >=0)
	{
		m_objectVis->setCubeColor(QColor(255, 5, 5, 255), cellID);
		//m_objectVis->setLinearCubeOffset(2.0);
		pickFibersinRegion(cellID);
	}
}

void iAVRMain::resetSelection()
{
	m_cylinderVis->renderSelection(std::vector<size_t>(), 0, QColor(140, 140, 140, 255), nullptr);
}

void iAVRMain::spawnModelInMiniature(double eventPosition[3], bool hide)
{
	
	if(!hide)
	{
		modelInMiniatureActive = true;
		double scale[3] = {0.15, 0.15, 0.15};
		int controllerID = static_cast<int>(vtkEventDataDevice::LeftController);

		updateModelInMiniatureData();
		m_objectVis->setScale(scale[0], scale[1], scale[2]);

		double sideDistance = *(m_objectVis->getActor()->GetYRange());
		double hoveringOffset[3] = { sideDistance * xOffset, sideDistance * yOffset, sideDistance * zOffset };
		m_objectVis->setPos(cPos[controllerID][0] + hoveringOffset[0], cPos[controllerID][1] + hoveringOffset[1], cPos[controllerID][2] + hoveringOffset[2]);

		m_objectVis->show();

		calculateMetrics();
		fiberMetrics->getColorBarLegend()->GetPositionCoordinate()->SetCoordinateSystemToWorld();
		fiberMetrics->getColorBarLegend()->GetPositionCoordinate()->SetValue(eventPosition);
		fiberMetrics->showColorBarLegend(m_vrEnv->renderer());
	}
	else
	{
		modelInMiniatureActive = false;
		m_objectVis->hide();
		fiberMetrics->hideColorBarLegend(m_vrEnv->renderer());
	}
}

