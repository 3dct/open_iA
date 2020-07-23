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
#include "iAVRModelInMiniature.h"
#include "iAVROctree.h"
#include "iAVRInteractorStyle.h"

#include "vtkRenderer.h"
#include "vtkIdList.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkTextProperty.h"
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
#include "vtkOpenVRCamera.h"
#include "vtkOutlineFilter.h"
#include <vtkTexture.h>
#include "vtkOpenVRModel.h"
#include <vtkArrowSource.h>
#include <vtkImageCanvasSource2D.h>

#include <QColor>

//Octree Max Level
#define OCTREE_MAX_LEVEL 3
#define OCTREE_MIN_LEVEL 1
#define OCTREE_POINTS_PER_REGION 1
#define OCTREE_COLOR QColor(126, 0, 223, 255)
//#define OCTREE_COLOR QColor(130, 10, 10, 255)

//Offsets for the hovering Effect of the Model in Miniature
#define X_OFFSET 0
#define Y_OFFSET 90
#define Z_OFFSET 0

iAVRMain::iAVRMain(iAVREnvironment* vrEnv, iAVRInteractorStyle* style, vtkTable* objectTable, iACsvIO io): m_vrEnv(vrEnv),
	m_style(style),	m_objectTable(objectTable),	m_io(io)
{
	currentMiMDisplacementType = 0;
	//m_cylinderVis = new iA3DCylinderObjectVis(m_vrEnv->renderer(), m_objectTable, m_io.getOutputMapping(), QColor(140,140,140,255), std::map<size_t, std::vector<iAVec3f> >());
	m_volume = new iAVRVolume(m_vrEnv->renderer(), m_objectTable, m_io);

	//m_cylinderVis->setShowLines(true);
	//m_cylinderVis->getActor()->GetProperty()->SetLineWidth(2);

	//Define Octree
	currentOctreeLevel = 0;
	m_octrees = new std::vector<iAVROctree*>();
	generateOctrees(OCTREE_MAX_LEVEL, OCTREE_POINTS_PER_REGION, m_volume->getVolumeData());
	m_octrees->at(currentOctreeLevel)->generateOctreeRepresentation(currentOctreeLevel, OCTREE_COLOR);
	m_octrees->at(currentOctreeLevel)->show();

	// For true TranslucentGeometry
	//https://vtk.org/Wiki/VTK/Examples/Cxx/Visualization/CorrectlyRenderTranslucentGeometry#CorrectlyRenderTranslucentGeometry.cxx
	//m_vrEnv->renderer()->SetUseDepthPeeling(true);
	//m_vrEnv->renderer()->SetMaximumNumberOfPeels(2);
	//m_vrEnv->renderer()->SetUseFXAA(true);

	//Create Cube
	m_modelInMiniature = new iAVRModelInMiniature(m_vrEnv->renderer());
	m_modelInMiniature->setOctree(m_octrees->at(OCTREE_MIN_LEVEL));

	//Initialize Metrics class
	currentFeature = 1;
	fiberMetrics = new iAVRMetrics(m_vrEnv->renderer(), m_objectTable, m_io, m_octrees);
	//map for the coverage of each fiber in every octree level and region
	m_fiberCoverage = new std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>();

	//Thread
	//m_iDMappingThread = std::thread(&iAVRMain::mapAllPointiDs, this);
	//m_iDMappingThread = std::thread(&iAVRMain::mapAllPointiDsAndCalculateFiberCoverage, this);
	mapAllPointiDsAndCalculateFiberCoverage();
	//m_iDMappingThreadRunning = false;

	//Add InteractorStyle
	m_style->setVRMain(this);
	m_vrEnv->interactor()->SetInteractorStyle(m_style);
	// Active Input Saves the current applied Input in case Multiinput is requires
	activeInput = m_style->getActiveInput();
	multiPickIDs = new std::vector<vtkIdType>();


	//Initialize Text Lables vector
	m_3DTextLabels = new std::vector<iAVR3DText*>();
	m_3DTextLabels->push_back(new iAVR3DText(m_vrEnv->renderer()));// [0] for Octree level change
	m_3DTextLabels->push_back(new iAVR3DText(m_vrEnv->renderer()));// [1] for feature change
	m_3DTextLabels->push_back(new iAVR3DText(m_vrEnv->renderer()));// [2] for Alerts

	//Initialize Dashboard
	m_dashboard = new iAVRDashboard(m_vrEnv);

	//Add Input Mapping
	//Press, Touch
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::Volume, iAVROperations::PickSingleFiber);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::MiniatureModel, iAVROperations::PickMiMRegion);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::ResetSelection);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::TrackPad, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ExplodeMiM);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::Volume, iAVROperations::PickFibersinRegion);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::TrackPad, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeOctreeAndMetric);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::ResetSelection);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::SpawnModelInMiniature);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::MiniatureModel, iAVROperations::SpawnModelInMiniature);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Grip, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::MultiPickMiMRegion);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Grip, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeMiMDisplacementType);
	//Release, Untouch
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Grip, vtkEventDataAction::Release,
		iAVRInteractionOptions::Anywhere, iAVROperations::MultiPickMiMRegion);

	//For Oculus
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Joystick, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeOctreeAndMetric);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Joystick, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ExplodeMiM);

	//m_cylinderVis->show();
	m_volume->setOctree(m_octrees->at(OCTREE_MIN_LEVEL));
	m_volume->showVolume();
	m_volume->show();

	//Add Actors
	addPropToOptionID(vtkProp3D::SafeDownCast(m_modelInMiniature->getActor()), iAVRInteractionOptions::MiniatureModel);
	addPropToOptionID(vtkProp3D::SafeDownCast(m_volume->getActor()), iAVRInteractionOptions::Volume);
	addPropToOptionID(vtkProp3D::SafeDownCast(m_volume->getVolumeActor()), iAVRInteractionOptions::Volume);
	for(int i = 0; i < m_octrees->size(); i++)
	{
		addPropToOptionID(vtkProp3D::SafeDownCast(m_octrees->at(i)->getActor()), iAVRInteractionOptions::Volume); //Octree counts as Volume
	}
	
}

//! Defines the action executed for specific controller inputs
//! Position and Orientation are in WorldCoordinates and Orientation is in Degree
void iAVRMain::startInteraction(vtkEventDataDevice3D* device, double eventPosition[3], double eventOrientation[4], vtkProp3D* pickedProp)
{
	m_vrEnv->interactor()->GetTouchPadPosition(device->GetDevice(), device->GetInput(), touchPadPosition);

	int deviceID = static_cast<int>(device->GetDevice()); // Device
	int inputID = static_cast<int>(device->GetInput());  // Input Method
	int actioniD = static_cast<int>(device->GetAction()); // Action of Input Method
	int optionID = getOptionForObject(pickedProp);

	//if (optionID == -1) return;

	inputScheme* scheme = m_style->getInputScheme();
	int operation = scheme->at(deviceID).at(inputID).at(actioniD).at(optionID);

	switch (iAVROperations(operation))
	{
	case iAVROperations::Unknown:
		DEBUG_LOG(QString("Unknown Operation!"));
		break;
	case iAVROperations::None:
		break;
	case iAVROperations::SpawnModelInMiniature:
		this->spawnModelInMiniature(eventPosition, modelInMiniatureActive);
		break;
	case iAVROperations::PickSingleFiber:
		this->pickSingleFiber(eventPosition);
		break;
	case iAVROperations::PickFibersinRegion:
		this->pickFibersinRegion(eventPosition, eventOrientation);
		break;
	case iAVROperations::PickMiMRegion:
		this->pickMimRegion(eventPosition, eventOrientation);
		break;
	case iAVROperations::MultiPickMiMRegion:	
		activeInput->at(deviceID) = static_cast<int>(iAVROperations::MultiPickMiMRegion); // For Multitouch
		break;
	case iAVROperations::ChangeOctreeAndMetric:
		this->changeOctreeAndMetric();
		break;
	case iAVROperations::ResetSelection:
		this->resetSelection();
		break;
	case iAVROperations::ExplodeMiM:
		this->explodeMiM(currentMiMDisplacementType, 25);
		break;
	case iAVROperations::ChangeMiMDisplacementType:
		this->ChangeMiMDisplacementType();
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
	//std::vector<int>* activeInput = m_style->getActiveInput();

	//if (optionID == -1) return;

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
	case iAVROperations::MultiPickMiMRegion:
		activeInput->at(deviceID) = 0; // For Multitouch
		multiPickMiMRegion();
		break;
	}
	//DEBUG_LOG(QString("[END] active Input rc = %1, lc = %2").arg(activeInput->at(1)).arg(activeInput->at(2)));
	//Update Changes //ToDO: Required? Or is render Selection enough?
	m_vrEnv->update();
}

void iAVRMain::onMove(vtkEventDataDevice3D * device, double movePosition[3], double eventOrientation[4], vtkProp3D * pickedProp)
{
	//Currently moved controller
	int deviceID = static_cast<int>(device->GetDevice());

	double oldFocalPoint[3] = {focalPoint[0], focalPoint[1], focalPoint[2]};
	double oldcPos[3] = { cPos[deviceID][0], cPos[deviceID][1], cPos[deviceID][2]};
	double oldcOrie[4] = { cOrie[deviceID][0], cOrie[deviceID][1], cOrie[deviceID][2], cOrie[deviceID][3]}; //W,X,Y,Z

	for (size_t i = 0; i < 3; i++)
	{
		//Save Current Pos
		cPos[deviceID][i] = movePosition[i];
		//Save Current Pos
		cOrie[deviceID][i] = eventOrientation[i];
	}
	cOrie[deviceID][3] = eventOrientation[3];

	double movementPos[3];
	double movementOrie[4];
	movementPos[0] = cPos[deviceID][0] - oldcPos[0];
	movementPos[1] = cPos[deviceID][1] - oldcPos[1];
	movementPos[2] = cPos[deviceID][2] - oldcPos[2];

	movementOrie[0] = cOrie[deviceID][0] - oldcOrie[0]; //W
	movementOrie[1] = cOrie[deviceID][1] - oldcOrie[1]; //X
	movementOrie[2] = cOrie[deviceID][2] - oldcOrie[2]; //Y
	movementOrie[3] = cOrie[deviceID][3] - oldcOrie[3]; //Z

	//Movement of Head
	if (deviceID == static_cast<int>(vtkEventDataDevice::HeadMountedDisplay))
	{
		double* headOrientation = m_vrEnv->renderer()->GetActiveCamera()->GetOrientation();

		//DEBUG_LOG(QString("Event Orie is: %1 / %2 / %3").arg(cOrie[deviceID][1]).arg(cOrie[deviceID][2]).arg(cOrie[deviceID][3]));
		//DEBUG_LOG(QString("> Head Orie is: %1 / %2 / %3").arg(headOrientation[0]).arg(headOrientation[1]).arg(headOrientation[2]));

		double* tempFocalPos = m_vrEnv->renderer()->GetActiveCamera()->GetFocalPoint();
		focalPoint[0] = tempFocalPos[0];
		focalPoint[1] = tempFocalPos[1];
		focalPoint[2] = tempFocalPos[2];

		double rotation[3] = { 0,0,0 };
		rotation[0] = oldFocalPoint[0] - focalPoint[0]; //X
		rotation[1] = oldFocalPoint[1] - focalPoint[1]; //Y
		rotation[2] = oldFocalPoint[2] - focalPoint[2]; //Z

		m_3DTextLabels->at(2)->setLabelPos(tempFocalPos);

		//fiberMetrics->rotateColorBarLegend(0, -movementOrie[0], 0);
	}

	//Movement of Left controller
	if(deviceID == static_cast<int>(vtkEventDataDevice::LeftController))
	{
		//For active Model in Minature MiM
		if(modelInMiniatureActive)
		{
			double* centerPos = m_modelInMiniature->getActor()->GetCenter();

			//Set Orientation!!	
			double* modellOrientation = m_modelInMiniature->getActor()->GetOrientationWXYZ();
			modellOrientation[1] = vtkMath::DegreesFromRadians(modellOrientation[1]);
			modellOrientation[2] = vtkMath::DegreesFromRadians(modellOrientation[2]);
			modellOrientation[3] = vtkMath::DegreesFromRadians(modellOrientation[3]);

			 //m_modelInMiniature->getActor()->AddOrientation(movementOrie[1] - modellOrientation[1], movementOrie[2] - modellOrientation[2], movementOrie[3] - modellOrientation[3]);
			 //m_modelInMiniature->getActor()->AddOrientation(cOrie[deviceID][1], cOrie[deviceID][2], cOrie[deviceID][3]);
			//m_modelInMiniature->getActor()->AddOrientation(0, movementOrie[2], 0);

			//m_modelInMiniature->getActor()->AddPosition(cPos[deviceID][0] - centerPos[0], cPos[deviceID][1] - centerPos[1], cPos[deviceID][2] - centerPos[2]);
			//m_modelInMiniature->getActor()->AddPosition(cPos[deviceID][0] - centerPos[0], cPos[deviceID][1] - centerPos[1], cPos[deviceID][2] - centerPos[2]);
			m_modelInMiniature->addPos(movementPos[0], movementPos[1], movementPos[2]);

			//DEBUG_LOG(QString("Event Orie is: %1 / %2 / %3").arg(cOrie[deviceID][1]).arg(cOrie[deviceID][2]).arg(cOrie[deviceID][3]));
			//DEBUG_LOG(QString("Miniature Modell Ori is: %1 / %2 / %3").arg(modellOrientation[1]).arg(modellOrientation[2]).arg(modellOrientation[3]));
			//DEBUG_LOG(QString("Movement Orie is: %1 / %2 / %3").arg(movementOrie[1]).arg(movementOrie[2]).arg(movementOrie[3]));
			//DEBUG_LOG(QString("Actors new Pos is: %1 / %2 / %3").arg(m_modelInMiniature->getActor()->GetPosition()[0]).arg(m_modelInMiniature->getActor()->GetPosition()[1]).arg(m_modelInMiniature->getActor()->GetPosition()[2]));
		
			double colorLegendlcPos[3] = { cPos[deviceID][0] + 70, cPos[deviceID][1] - 50, cPos[deviceID][2] + 10};
			fiberMetrics->moveColorBarLegend(colorLegendlcPos);
			fiberMetrics->showColorBarLegend();
		}

		double lcPos[3] = { cPos[deviceID][0], cPos[deviceID][1] - 24, cPos[deviceID][2]};
		m_3DTextLabels->at(1)->setLabelPos(lcPos);
		m_3DTextLabels->at(1)->moveInEyeDir(25, 25, 25);
	}

	//Movement of Right controller
	if (deviceID == static_cast<int>(vtkEventDataDevice::RightController))
	{
		double rcPos[3] = { cPos[deviceID][0], cPos[deviceID][1] - 22, cPos[deviceID][2]};
		m_3DTextLabels->at(0)->setLabelPos(rcPos);
		m_3DTextLabels->at(0)->moveInEyeDir(24, 24, 24);
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

		// Insert polyObject ID of Start Point and End Point
		m_pointIDToCsvIndex.insert(std::make_pair(m_volume->getVolumeData()->FindPoint(startPos), row));
		m_pointIDToCsvIndex.insert(std::make_pair(m_volume->getVolumeData()->FindPoint(endPos), row));

		// Insert fiber id with its Start Point and End Point
		m_csvIndexToPointID.insert(std::make_pair(row, m_volume->getVolumeData()->FindPoint(startPos)));
		m_csvIndexToPointID.insert(std::make_pair(row, m_volume->getVolumeData()->FindPoint(endPos)));
	}
	DEBUG_LOG(QString("Volume Data loaded"));

	//Calculate Fibers in Region
	for (int i = 0; i < m_octrees->size(); i++)
	{
		m_fiberCoverage->push_back(*m_octrees->at(i)->getfibersInRegionMapping(&m_pointIDToCsvIndex));
	}
	DEBUG_LOG(QString("Fibers in Region for every octree calculated"));

	fiberMetrics->setFiberCoverageData(m_fiberCoverage);
	m_iDMappingThreadRunning = false; //Thread ended
}

//! Computes which polyObject ID (points) belongs to which Object ID in the csv file of the volume for every Octree Level
//! Calculates Intersection between points in different Octree regions in all levels. Calls a emthod to compute fiber coverage.
//! Gets only called internally from thread to store the mapping
void iAVRMain::mapAllPointiDsAndCalculateFiberCoverage()
{

	//m_extendedCylinderVisData = vtkSmartPointer<vtkPolyData>::New();
	//m_extendedCylinderVisData = m_cylinderVis->getPolyData();
	//m_extendedCylinderVisData->DeepCopy(m_cylinderVis->getPolyData()); // Use copy of volume data
	int count = 0;

	//Initialize new Vectors
	for (int level = 0; level < m_octrees->size(); level++)
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

		double startPos[3], endPos[3];
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
		for (int level = 0; level < m_octrees->size(); level++)
		{

			//Skip intersection test on lowest Octree level
			if(level == 0)
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
					DEBUG_LOG(QString("!! vtkPoints is null..."));
				}
			}
			//ADD the new Points to the Polydata and the Mapping
			//m_extendedCylinderVisData->GetPoints()->InsertNextPoint(intersectionPoint);
			// bzw. m_extendedCylinderVisData->SetPoints(intersectionPoints)
			//m_extendedCylinderVisData->GetPoints()->Modified();
			//m_pointIDToCsvIndex.insert(std::make_pair(m_extendedCylinderVisData->FindPoint(intersectionPoint), row));
	
		}
		
	}

	//m_3DTextLabels->at(2)->create3DLabel("Volume loading has finished!");
	//m_3DTextLabels->at(2)->show();
	m_volume->setMappers(m_pointIDToCsvIndex, m_csvIndexToPointID);
	m_volume->setFiberCoverageData(m_fiberCoverage);
	m_modelInMiniature->setFiberCoverageData(m_fiberCoverage);
	fiberMetrics->setFiberCoverageData(m_fiberCoverage);

	for (int level = 1; level < m_octrees->size(); level++)
	{
		m_octrees->at(level)->getRegionsInLineOfRay();
	}

	DEBUG_LOG(QString("Volume Data loaded and Intersection test finished"));
	m_iDMappingThreadRunning = false; //Thread ended
}

//! The calculated intersection points are returned as vtkPoints
vtkSmartPointer<vtkPoints> iAVRMain::getOctreeFiberCoverage(double startPoint[3], double endPoint[3], int octreeLevel, int fiber, double fiberLength)
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

	for (int region = 0; region < leafNodes; region++)
	{
		double lastIntersection[3] = { -1, -1, -1 };
		int pointsInRegion = 0;
		double bounds[6];
		std::vector<std::vector<iAVec3d>>* planePoints = new std::vector<std::vector<iAVec3d>>();
		m_octrees->at(octreeLevel)->createOctreeBoundingBoxPlanes(region, planePoints);
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
				if (checkIntersectionWithBox(startPoint, endPoint, planePoints, bounds, intersectionPoint))
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
				if (checkIntersectionWithBox(endPoint, startPoint, planePoints, bounds, intersectionPoint))
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
					if (checkIntersectionWithBox(lastIntersection, endPoint, planePoints, bounds, intersectionPoint))
					{
						additionalIntersectionPoints->InsertNextPoint(lastIntersection);
						additionalIntersectionPoints->InsertNextPoint(intersectionPoint);
						double coverage = calculateFiberCoverage(lastIntersection, intersectionPoint, fiberLength);
						pointsInRegion = 2;
						m_fiberCoverage->at(octreeLevel).at(region)->insert(std::make_pair(fiber, coverage));
						break;
					}
					else if (checkIntersectionWithBox(lastIntersection, startPoint, planePoints, bounds, intersectionPoint))
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
					
					if (checkIntersectionWithBox(startPoint, endPoint, planePoints, bounds, intersectionPoint))
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
			return static_cast<int>(iAVRInteractionOptions::NoInteractions);
		}
		return m_ActorToOptionID.at(pickedProp);
	}
}

void iAVRMain::addPropToOptionID(vtkProp3D* prop, iAVRInteractionOptions iD)
{
	m_ActorToOptionID.insert(std::make_pair(prop, static_cast<int>(iD)));
}

bool iAVRMain::checkIntersectionWithBox(double startPoint[3], double endPoint[3], std::vector<std::vector<iAVec3d>>* planePoints, double bounds[6], double intersection[3])
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
			if((t > 0) && (t <= ray.length()) &&
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

double iAVRMain::calculateFiberCoverage(double startPoint[3], double endPoint[3], double fiberLength)
{
	double vectorBetweenStartAndEnd[3];
	for(int i = 0; i < 3; i++)
	{
		vectorBetweenStartAndEnd[i] = (endPoint[i] - startPoint[i]);
	}

	double coverage = sqrt(pow(vectorBetweenStartAndEnd[0], 2) + pow(vectorBetweenStartAndEnd[1], 2) + pow(vectorBetweenStartAndEnd[2], 2));
	double ratio = coverage / fiberLength;

	return ratio;
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
	pointsActor->GetProperty()->SetPointSize(9);
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
		fiberMetrics->hideColorBarLegend();
		fiberMetrics->hideMIPPanels();

		std::vector<QColor>* rgba = fiberMetrics->getHeatmapColoring(currentOctreeLevel, currentFeature, 1);

		if (modelInMiniatureActive) 
		{
			m_modelInMiniature->applyHeatmapColoring(rgba); // Only call when model is calculated (poly data has to be accessible)

			QString text = QString("Feature: %1").arg(fiberMetrics->getFeatureName(currentFeature));
			m_3DTextLabels->at(1)->create3DLabel(text);

			fiberMetrics->setLegendTitle(QString(" %1 ").arg(fiberMetrics->getFeatureName(currentFeature)).toUtf8());

			fiberMetrics->createMIPPanels(currentOctreeLevel, currentFeature);
		}
	}
}

void iAVRMain::colorMiMCubes(std::vector<vtkIdType>* regionIDs)
{
	if (!m_iDMappingThreadRunning) {
		
		std::vector<QColor>* rgba = fiberMetrics->getHeatmapColoring(currentOctreeLevel, currentFeature, 1);
		m_modelInMiniature->applyHeatmapColoring(rgba); //Reset Color
	
		for (int i = 0; i < regionIDs->size(); i++)
		{
			m_modelInMiniature->setCubeColor(OCTREE_COLOR, regionIDs->at(i));
		}
	}
}

//! Increases/Decreases the current octree level and feature. Recalculates the Model in Miniature Object.
void iAVRMain::changeOctreeAndMetric()
{
	iAVRTouchpadPosition touchpadPos = m_style->getTouchedPadSide(touchPadPosition);

	if (touchpadPos == iAVRTouchpadPosition::Up || touchpadPos == iAVRTouchpadPosition::Down) {

		m_octrees->at(currentOctreeLevel)->hide(); // Hide old octree
		m_volume->hide();
		m_volume->hideRegionLinks();
		m_volume->hideVolume();
		m_volume->resetVolume();
		addPropToOptionID(vtkProp3D::SafeDownCast(m_volume->getVolumeActor()), iAVRInteractionOptions::Volume);

		if (touchpadPos == iAVRTouchpadPosition::Up)
		{
			if (currentOctreeLevel < m_octrees->size() - 1)	currentOctreeLevel++;
		}
		if (touchpadPos == iAVRTouchpadPosition::Down)
		{
			if (currentOctreeLevel > OCTREE_MIN_LEVEL)	currentOctreeLevel--;
		}

		QString text = QString("Octree Level %1").arg(currentOctreeLevel);
		m_3DTextLabels->at(0)->create3DLabel(text);
		m_3DTextLabels->at(0)->show();
		m_3DTextLabels->at(1)->hide();

		//m_octrees->at(currentOctreeLevel)->generateOctreeRepresentation(currentOctreeLevel, OCTREE_COLOR);
		//m_octrees->at(currentOctreeLevel)->show();

		m_volume->setOctree(m_octrees->at(currentOctreeLevel));
		m_volume->createCubeModel();
		m_volume->show();
		m_volume->showVolume();
	}
	else if (touchpadPos == iAVRTouchpadPosition::Right || touchpadPos == iAVRTouchpadPosition::Left){
	
		if (touchpadPos == iAVRTouchpadPosition::Right)
		{
			if (currentFeature < fiberMetrics->getNumberOfFeatures() - 1)	currentFeature++;
			//m_octrees->at(currentOctreeLevel)->show();
		}
		if (touchpadPos == iAVRTouchpadPosition::Left)
		{
			if (currentFeature > 0)	currentFeature--;
			//m_octrees->at(currentOctreeLevel)->hide();
		}

		m_3DTextLabels->at(1)->show();
		m_3DTextLabels->at(0)->hide();
	}
	updateModelInMiniatureData();
}

void iAVRMain::pickSingleFiber(double eventPosition[3])
{
	std::vector<size_t> selection = std::vector<size_t>();
	// Find the closest points to TestPoint
	vtkIdType iD = m_octrees->at(currentOctreeLevel)->getOctree()->FindClosestPoint(eventPosition);

	vtkIdType rowiD = getObjectiD(iD);
	selection.push_back(rowiD);

	//double pos[3];
	//m_octrees->at(currentOctreeLevel)->getOctree()->GetDataSet()->GetPoint(iD, pos); //For Debug
	//DEBUG_LOG(QString("Fiber %1 in Region %2").arg(rowiD).arg(m_octrees->at(currentOctreeLevel)->getOctree()->GetRegionContainingPoint(pos[0], pos[1], pos[2])));

	m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
}

//! Picks all fibers in the region clicked by the user.
void iAVRMain::pickFibersinRegion(double eventPosition[3], double eventOrientation[4])
{
	vtkIdType cellID = m_modelInMiniature->getClosestCellID(eventPosition, eventOrientation);

	//ToDO current fiber cells instead of only region cells can be detected (use picking List?)
	if (cellID >= 0 && cellID <= m_octrees->at(currentOctreeLevel)->getNumberOfLeafeNodes())
	{
		DEBUG_LOG(QString("Picked region %1 :").arg(cellID));
		pickFibersinRegion(cellID);
	}
}

//! Picks all fibers in the octree region defined by the leaf node ID.
void iAVRMain::pickFibersinRegion(int leafRegion)
{
	std::vector<size_t> selection = std::vector<size_t>();

	
	for (auto fiber : *m_fiberCoverage->at(currentOctreeLevel).at(leafRegion)) {
		//DEBUG_LOG(QString("Nr. [%1]").arg(fiber.first));
		selection.push_back(fiber.first);
	}
	std::sort(selection.begin(), selection.end());

	multiPickIDs->push_back(leafRegion);
	m_modelInMiniature->highlightGlyphs(multiPickIDs);
	m_volume->highlightGlyphs(multiPickIDs);

	multiPickIDs->clear();
	m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
}

void iAVRMain::pickMimRegion(double eventPosition[3], double eventOrientation[4])
{
	vtkIdType cellID = m_modelInMiniature->getClosestCellID(eventPosition, eventOrientation);

	if (cellID >= 0)
	{
		multiPickIDs->push_back(cellID);
		//colorMiMCubes(multiPickIDs);
		m_modelInMiniature->highlightGlyphs(multiPickIDs);

		// If multitouch Key is not pressed render the single region it in the Volume
		if(activeInput->at(static_cast<int>(vtkEventDataDevice::RightController)) != static_cast<int>(iAVROperations::MultiPickMiMRegion)){
			pickFibersinRegion(multiPickIDs->at(0));
			multiPickIDs->clear();
		}
	}

}

//! Methods ends the multi picking mode and renders selection
void iAVRMain::multiPickMiMRegion()
{	
	if(!multiPickIDs->empty())
	{
		std::vector<size_t> selection = std::vector<size_t>();

		for (int i = 0; i < multiPickIDs->size(); i++)
		{
			for (auto fiber : *m_fiberCoverage->at(currentOctreeLevel).at(multiPickIDs->at(i))) {
				selection.push_back(fiber.first);
			}
		}

		std::sort(selection.begin(), selection.end());
		selection.erase(std::unique(selection.begin(), selection.end()), selection.end());

		m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);

		multiPickIDs->clear();
	}
}

void iAVRMain::resetSelection()
{
	m_volume->renderSelection(std::vector<size_t>(), 0, QColor(140, 140, 140, 255), nullptr);
	if(modelInMiniatureActive)
	{
		std::vector<QColor>* rgba = fiberMetrics->getHeatmapColoring(currentOctreeLevel, currentFeature, 1);
		m_modelInMiniature->applyHeatmapColoring(rgba); //Reset Color
		m_modelInMiniature->removeHighlightedGlyphs();
		m_volume->removeHighlightedGlyphs();
		//fiberMetrics->hideMIPPanels();
	}
}

//! Updates the data (Octree, Metrics,...) and the position for the current MiM
void iAVRMain::updateModelInMiniatureData()
{
	int controllerID = static_cast<int>(vtkEventDataDevice::LeftController);
	
	m_modelInMiniature->setOctree(m_octrees->at(currentOctreeLevel));
	m_modelInMiniature->createCubeModel(); //Here a new MiM is calculated

	double* centerPos = m_modelInMiniature->getActor()->GetCenter();
	//double scale = m_vrEnv->interactor()->GetPhysicalScale();
	//double shiftY = Y_OFFSET / scale;
	m_modelInMiniature->getActor()->AddPosition(cPos[controllerID][0] - centerPos[0], cPos[controllerID][1] - centerPos[1] + Y_OFFSET, cPos[controllerID][2] - centerPos[2]);
	//m_modelInMiniature->getActor()->Modified();

	calculateMetrics();
}

void iAVRMain::spawnModelInMiniature(double eventPosition[3], bool hide)
{
	if(!hide)
	{
		modelInMiniatureActive = true;
		updateModelInMiniatureData();

		m_modelInMiniature->show();

		fiberMetrics->moveColorBarLegend(eventPosition);
		fiberMetrics->showColorBarLegend();
	}
	else
	{
		modelInMiniatureActive = false;
		m_modelInMiniature->hide();
		m_modelInMiniature->removeHighlightedGlyphs();
		m_volume->removeHighlightedGlyphs();
		fiberMetrics->hideColorBarLegend();
	}
}

void iAVRMain::explodeMiM(int currentMiMDisplacementType, double offset)
{
	if(modelInMiniatureActive && currentOctreeLevel > 0)
	{
		switch (m_style->getTouchedPadSide(touchPadPosition))
		{
		case iAVRTouchpadPosition::Up:
			//Just use positive offset
			break;
		case iAVRTouchpadPosition::Down:
			offset = offset * -1;
			break;
		case iAVRTouchpadPosition::Left:
			m_volume->show();
			m_volume->renderSelection(std::vector<size_t>(), 0, QColor(140, 140, 140, 255), nullptr);
			m_volume->hideRegionLinks();
			return;
		case iAVRTouchpadPosition::Right:
			m_volume->hide();
			m_volume->renderSelection(std::vector<size_t>(1,0), 0, QColor(140, 140, 140, 32), nullptr);
			m_volume->showRegionLinks();
			return;
		}

		switch (currentMiMDisplacementType)
		{
		case -1:
			DEBUG_LOG(QString("Unknown Displacement!"));
			break;
		case 0:
			m_modelInMiniature->applyRelativeCubeOffset(offset);
			m_volume->applyRelativeCubeOffset(offset);
			m_volume->moveFibersByMaxCoverage(fiberMetrics->getMaxCoverageFiberPerRegion(), offset);
			break;
		case 1:
			m_modelInMiniature->applyLinearCubeOffset(offset);
			m_volume->applyLinearCubeOffset(offset);
			m_volume->moveFibersByMaxCoverage(fiberMetrics->getMaxCoverageFiberPerRegion(), offset);
			//m_volume->moveFibersbyAllCoveredRegions(offset);
			break;
		case 2:
			m_modelInMiniature->apply4RegionCubeOffset(offset);
			m_volume->apply4RegionCubeOffset(offset);
			break;
		}
		fiberMetrics->hideMIPPanels();
		m_volume->createRegionLinks(fiberMetrics->getWeightedJaccardIndex(currentOctreeLevel), fiberMetrics->getMaxNumberOfFibersInRegion(currentOctreeLevel));
	}
}

void iAVRMain::ChangeMiMDisplacementType()
{
	currentMiMDisplacementType++;

	if(currentMiMDisplacementType > 2)
	{
		currentMiMDisplacementType = 0;
	}

	switch (currentMiMDisplacementType)
	{
	case 0:
		m_3DTextLabels->at(1)->create3DLabel(QString("RelativeCubeOffset"));
		break;
	case 1:
		m_3DTextLabels->at(1)->create3DLabel(QString("LinearCubeOffset"));
		break;
	case 2:
		m_3DTextLabels->at(1)->create3DLabel(QString("4RegionCubeOffset"));
		break;
	}

	m_3DTextLabels->at(1)->show();
	m_3DTextLabels->at(0)->hide();	
}

