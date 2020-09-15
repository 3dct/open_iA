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
#include "iAVRSlider.h"

#include "vtkRenderer.h"
#include "vtkIdList.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkTextProperty.h"
#include "vtkActor.h"
#include "vtkActorCollection.h"
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
#define OCTREE_MIN_LEVEL 0
#define OCTREE_POINTS_PER_REGION 1
#define OCTREE_COLOR QColor(126, 0, 223, 255)
//#define OCTREE_COLOR QColor(130, 10, 10, 255)

iAVRMain::iAVRMain(iAVREnvironment* vrEnv, iAVRInteractorStyle* style, vtkTable* objectTable, iACsvIO io): m_vrEnv(vrEnv),
	m_style(style),	m_objectTable(objectTable),	m_io(io)
{
	currentMiMDisplacementType = 0;
	//m_cylinderVis = new iA3DCylinderObjectVis(m_vrEnv->renderer(), m_objectTable, m_io.getOutputMapping(), QColor(140,140,140,255), std::map<size_t, std::vector<iAVec3f> >());
	m_volume = new iAVRVolume(m_vrEnv->renderer(), m_objectTable, m_io);

	//Define Octree
	currentOctreeLevel = 0;
	m_octrees = new std::vector<iAVROctree*>();
	generateOctrees(OCTREE_MAX_LEVEL, OCTREE_POINTS_PER_REGION, m_volume->getVolumeData());
	//m_octrees->at(currentOctreeLevel)->generateOctreeRepresentation(currentOctreeLevel, OCTREE_COLOR);
	//m_octrees->at(currentOctreeLevel)->show();

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

	//Initialize Slider
	m_slider = new iAVRSlider(m_vrEnv->renderer(), m_vrEnv->interactor());

	//Initialize Distribution Vis
	m_networkGraphMode = false;
	m_distributionVis = new iAVRDistributionVis(m_vrEnv->renderer(), fiberMetrics, m_objectTable, m_io);

	//Add Input Mapping
	//Press, Touch
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::Volume, iAVROperations::PickFibersinRegion);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::MiniatureModel, iAVROperations::PickMiMRegion);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::ResetSelection);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::Histogram, iAVROperations::RotateVis);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeMiMDisplacementType);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::TrackPad, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeJaccardIndex);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::TrackPad, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ExplodeMiM);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::TrackPad, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeOctreeAndMetric);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::DisplayNodeLinkDiagram);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::SpawnModelInMiniature);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::MiniatureModel, iAVROperations::SpawnModelInMiniature);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Grip, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::MultiPickMiMRegion);

	//Release, Untouch
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Grip, vtkEventDataAction::Release,
		iAVRInteractionOptions::Anywhere, iAVROperations::MultiPickMiMRegion);

	//For Oculus
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Joystick, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeOctreeAndMetric);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Joystick, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ExplodeMiM);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Joystick, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeJaccardIndex);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Release,
		iAVRInteractionOptions::Histogram, iAVROperations::RotateVis);

	//m_cylinderVis->show();
	m_volume->setOctree(m_octrees->at(OCTREE_MIN_LEVEL));
	m_volume->createCubeModel();
	m_volume->showVolume();
	m_volume->show();

	//Add Actors
	addPropToOptionID(vtkProp3D::SafeDownCast(m_modelInMiniature->getActor()), iAVRInteractionOptions::MiniatureModel);
	addPropToOptionID(vtkProp3D::SafeDownCast(m_volume->getActor()), iAVRInteractionOptions::Volume);
	addPropToOptionID(vtkProp3D::SafeDownCast(m_volume->getVolumeActor()), iAVRInteractionOptions::Volume);

	//addPropToOptionID(m_slider->getSlider(), iAVRInteractionOptions::Histogram);
	
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
	
	//DEBUG_LOG(QString("Object Number: %1").arg(optionID));
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
	case iAVROperations::PickFibersinRegion:
		this->pickFibersinRegion(eventPosition, eventOrientation);
		break;
	case iAVROperations::PickMiMRegion:
		this->pickMimRegion(eventPosition, eventOrientation);
		break;
	case iAVROperations::MultiPickMiMRegion:	
		activeInput->at(deviceID) = static_cast<int>(iAVROperations::MultiPickMiMRegion); // For Multitouch
		//m_modelInMiniature->removeHighlightedGlyphs();
		multiPickIDs->clear();
		break;
	case iAVROperations::ChangeOctreeAndMetric:
		this->changeOctreeAndMetric();
		break;
	case iAVROperations::ResetSelection:
		this->resetSelection();
		break;
	case iAVROperations::ExplodeMiM:
		this->pressLeftTouchpad();
		break;
	case iAVROperations::ChangeMiMDisplacementType:
		this->changeMiMDisplacementType();
		break;
	case iAVROperations::DisplayNodeLinkDiagram:
		this->displayNodeLinkD();
		break;
	case iAVROperations::ChangeJaccardIndex:
		this->pressLeftTouchpad();
		break;
	case iAVROperations::RotateVis:
		activeInput->at(deviceID) = static_cast<int>(iAVROperations::RotateVis);
		this->rotateDistributionVis(eventPosition, true);
		break;
	}

	//DEBUG_LOG(QString("[START] active Input rc = %1, lc = %2").arg(activeInput->at(1)).arg(activeInput->at(2)));
	//Update Changes //ToDO: Required? Or is render Selection enough?
	//m_vrEnv->update();
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
	case iAVROperations::RotateVis:
		this->rotateDistributionVis(eventPosition, false);
		activeInput->at(deviceID) = 0;
		break;
	}
	m_vrEnv->update();
}

void iAVRMain::onMove(vtkEventDataDevice3D * device, double movePosition[3], double eventOrientation[4])
{
	double initialScale = m_vrEnv->interactor()->GetPhysicalScale();
	//Currently moved controller
	int deviceID = static_cast<int>(device->GetDevice());

	//TODO Initialize only after first call or they are filled wrong!
	int oldViewDirection = viewDirection;
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
		double* tempFocalPos = m_vrEnv->renderer()->GetActiveCamera()->GetFocalPoint();
		focalPoint[0] = tempFocalPos[0];
		focalPoint[1] = tempFocalPos[1];
		focalPoint[2] = tempFocalPos[2];

		double rotation[3] = { 0,0,0 };
		rotation[0] = oldFocalPoint[0] - focalPoint[0]; //X
		rotation[1] = oldFocalPoint[1] - focalPoint[1]; //Y
		rotation[2] = oldFocalPoint[2] - focalPoint[2]; //Z

		m_3DTextLabels->at(2)->setLabelPos(tempFocalPos);

		double* tempViewDirection = m_vrEnv->renderer()->GetActiveCamera()->GetDirectionOfProjection();
		viewDirection = static_cast<int>(m_style->getViewDirection(tempViewDirection));

		if (m_networkGraphMode)
		{
			//Calc half distance of focalPoint vec
			iAVec3d focalP = iAVec3d(tempFocalPos);
			iAVec3d viewDir = iAVec3d(tempViewDirection);
			viewDir = viewDir * (m_vrEnv->renderer()->GetActiveCamera()->GetDistance() / 1.5);
			focalP = focalP - viewDir;

			m_distributionVis->determineHistogramInView(focalP.data());
		}

		//Create MIP panel in View direction
		if(oldViewDirection != viewDirection)
		{
			if (modelInMiniatureActive)
			{
				if(m_MIPPanelsVisible)
				{
					fiberMetrics->createSingleMIPPanel(currentOctreeLevel, currentFeature, viewDirection);
				}
			}
		}
	}

	//Movement of Left controller
	if(deviceID == static_cast<int>(vtkEventDataDevice::LeftController))
	{
		//For active Model in Minature MiM
		if(modelInMiniatureActive)
		{
			m_modelInMiniature->setPos(cPos[deviceID][0] - (initialScale * 0.08), cPos[deviceID][1] + (initialScale * 0.075), cPos[deviceID][2] - (initialScale * 0.085));

			double colorLegendlcPos[3] = { cPos[deviceID][0] + (initialScale * 0.04), cPos[deviceID][1] - (initialScale * 0.2), cPos[deviceID][2]};
			fiberMetrics->moveColorBarLegend(colorLegendlcPos);
			fiberMetrics->setLegendTitle(QString(" %1 ").arg(fiberMetrics->getFeatureName(currentFeature)).toUtf8());
			fiberMetrics->showColorBarLegend();
		}

		double lcPos[3] = { cPos[deviceID][0], cPos[deviceID][1] - (initialScale * 0.03), cPos[deviceID][2]};
		m_3DTextLabels->at(1)->setLabelPos(lcPos);
		m_3DTextLabels->at(1)->moveInEyeDir(initialScale * 0.04, initialScale * 0.04, initialScale * 0.04);

		m_slider->setPosition(lcPos[0], lcPos[1] - 22, lcPos[2] + 70);
	}

	//Movement of Right controller
	if (deviceID == static_cast<int>(vtkEventDataDevice::RightController))
	{
		double rcPos[3] = { cPos[deviceID][0], cPos[deviceID][1] - (initialScale * 0.03), cPos[deviceID][2]};
		m_3DTextLabels->at(0)->setLabelPos(rcPos);
		m_3DTextLabels->at(0)->moveInEyeDir(initialScale * 0.04, initialScale * 0.04, initialScale * 0.04);

		if(activeInput->at(deviceID) == static_cast<int>(iAVROperations::RotateVis))
		{
			double controllerPath[3]{};
			double crossPro[3]{};
			vtkMath::Subtract(oldcPos,cPos[deviceID], controllerPath);
			vtkMath::Cross(oldcPos, cPos[deviceID], crossPro);
			iAVec3d length = iAVec3d(controllerPath);
			controllerTravelledDistance = length.length();

			m_rotationOfDisVis = vtkMath::DegreesFromRadians(2 * asin(controllerTravelledDistance / (2 * m_distributionVis->getRadius())));
			sign = crossPro[1] < 0 ? -1 : 1;
			m_rotationOfDisVis *= sign;

		}
	}
}

void iAVRMain::onZoom()
{
	auto scaleDiff = (1.0 / calculateWorldScaleFactor());

	m_modelInMiniature->setScale(scaleDiff, scaleDiff, scaleDiff);
	fiberMetrics->resizeColorBarLegend(scaleDiff);
}

void iAVRMain::onRotate(double angle)
{
	//vtkSmartPointer<vtkActorCollection> actors = m_vrEnv->renderer()->GetActors();
	//actors->InitTraversal();

	//for (vtkIdType i = 0; i < actors->GetNumberOfItems(); i++)
	//{
	//	vtkActor* nextActor = actors->GetNextActor();
	//	if (nextActor != nullptr)
	//	{
	//		nextActor->RotateY(-angle);
	//	}
	//}
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
//! Gets only called internally
void iAVRMain::mapAllPointiDsAndCalculateFiberCoverage()
{
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

bool iAVRMain::checkEqualArrays(double pos1[3], double pos2[3])
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
	m_vrEnv->renderer()->RemoveActor(pointsActor);

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
	
	pointsActor = vtkSmartPointer<vtkActor>::New();
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

	fiberMetrics->hideColorBarLegend();
	fiberMetrics->hideMIPPanels();

	std::vector<QColor>* rgba = fiberMetrics->getHeatmapColoring(currentOctreeLevel, currentFeature, 1);
	fiberMetrics->calculateColorBarLegend(m_vrEnv->getInitialWorldScale());

	if (modelInMiniatureActive) 
	{
		m_modelInMiniature->applyHeatmapColoring(rgba); // Only call when model is calculated (poly data has to be accessible)

		QString text = QString("Feature: %1").arg(fiberMetrics->getFeatureName(currentFeature));
		m_3DTextLabels->at(1)->create3DLabel(text);

		fiberMetrics->setLegendTitle(QString(" %1 ").arg(fiberMetrics->getFeatureName(currentFeature)).toUtf8());

		m_MIPPanelsVisible = true;
		fiberMetrics->createSingleMIPPanel(currentOctreeLevel, currentFeature,viewDirection);
	}
	
}

void iAVRMain::colorMiMCubes(std::vector<vtkIdType>* regionIDs)
{

	std::vector<QColor>* rgba = fiberMetrics->getHeatmapColoring(currentOctreeLevel, currentFeature, 1);
	fiberMetrics->calculateColorBarLegend(m_vrEnv->getInitialWorldScale());
	m_modelInMiniature->applyHeatmapColoring(rgba); //Reset Color
	
	for (int i = 0; i < regionIDs->size(); i++)
	{
		m_modelInMiniature->setCubeColor(OCTREE_COLOR, regionIDs->at(i));
	}
	
}

//! Returns the difference between the intial world scaling and the current scaling
double iAVRMain::calculateWorldScaleFactor()
{
	double currentScale = m_vrEnv->interactor()->GetPhysicalScale();
	auto temp = m_vrEnv->getInitialWorldScale()/ currentScale;

	return temp;
}

//! Increases/Decreases the current octree level and feature. Recalculates the Model in Miniature Object.
void iAVRMain::changeOctreeAndMetric()
{
	iAVRTouchpadPosition touchpadPos = m_style->getTouchedPadSide(touchPadPosition);

	if (touchpadPos == iAVRTouchpadPosition::Up || touchpadPos == iAVRTouchpadPosition::Down) {

		//m_octrees->at(currentOctreeLevel)->hide(); // Hide old octree
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
		m_networkGraphMode = false;
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

	if (cellID >= 0 && cellID <= m_octrees->at(currentOctreeLevel)->getNumberOfLeafeNodes())
	{
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
	if(modelInMiniatureActive) m_modelInMiniature->highlightGlyphs(multiPickIDs);
	m_volume->highlightGlyphs(multiPickIDs);

	multiPickIDs->clear();
	m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
}

void iAVRMain::pickMimRegion(double eventPosition[3], double eventOrientation[4])
{
	vtkIdType cellID = m_modelInMiniature->getClosestCellID(eventPosition, eventOrientation);

	if (cellID >= 0)
	{
		auto keyPos = std::find(multiPickIDs->begin(), multiPickIDs->end(), cellID);
		//If the currently selected Region is already selected, then remove it (UNDO)
		if (keyPos != multiPickIDs->end()) multiPickIDs->erase(keyPos);
		else multiPickIDs->push_back(cellID);
		
		m_modelInMiniature->highlightGlyphs(multiPickIDs);
		m_volume->highlightGlyphs(multiPickIDs);

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

		//If not Network Mode then render fibers
		if(!m_networkGraphMode)	m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
		else
		{
			m_distributionVis->hide();
			multiPickIDs->resize(2); //only two regions allowed

			std::vector<int>* featureList = new std::vector<int>(); //
			//featureList->push_back(currentFeature); //
			featureList->push_back(22);
			featureList->push_back(21);
			featureList->push_back(20);
			featureList->push_back(11);
			featureList->push_back(9);
			featureList->push_back(8);
			featureList->push_back(7);

			auto cubePos = m_volume->getCubePos(multiPickIDs->at(0));
			auto cubeSize = m_volume->getCubeSize(multiPickIDs->at(0));
			auto visSize = m_vrEnv->interactor()->GetPhysicalScale() * 0.33; //33%
			
			m_distributionVis->createVisualization(cubePos, visSize, ceil(cubeSize), currentOctreeLevel, multiPickIDs, featureList); //
			m_distributionVis->show(); 
			m_distributionVis->rotateVisualization(180); //Initial Rotation
			m_volume->removeHighlightedGlyphs();
			m_volume->hideVolume();
			m_modelInMiniature->removeHighlightedGlyphs();

			auto coloring = m_distributionVis->getBarColors();
			m_volume->setNodeColor(*multiPickIDs, coloring);
			m_modelInMiniature->highlightGlyphs(multiPickIDs, &coloring);
			
			addPropToOptionID(vtkProp3D::SafeDownCast(m_distributionVis->getVisAssembly()), iAVRInteractionOptions::Histogram);//
		}
		multiPickIDs->clear();
	}
}

void iAVRMain::resetSelection()
{
	m_volume->renderSelection(std::vector<size_t>(), 0, QColor(140, 140, 140, 255), nullptr);
	m_volume->removeHighlightedGlyphs();
	if(modelInMiniatureActive)
	{
		std::vector<QColor>* rgba = fiberMetrics->getHeatmapColoring(currentOctreeLevel, currentFeature, 1);
		fiberMetrics->calculateColorBarLegend(m_vrEnv->getInitialWorldScale());
		m_modelInMiniature->applyHeatmapColoring(rgba); //Reset Color
		m_volume->resetNodeColor();
		m_modelInMiniature->removeHighlightedGlyphs();
		//fiberMetrics->hideMIPPanels();
	}
	m_distributionVis->hide();
}

//! Updates the data (Octree, Metrics,...) and the position for the current MiM
void iAVRMain::updateModelInMiniatureData()
{
	int controllerID = static_cast<int>(vtkEventDataDevice::LeftController);
	
	//m_networkGraphMode = false;
	resetSelection();

	m_modelInMiniature->setOctree(m_octrees->at(currentOctreeLevel));
	m_modelInMiniature->createCubeModel(); //Here a new MiM is calculated

	//double offset = m_vrEnv->getInitialWorldScale() * 0.15;
	//double* centerPos = m_modelInMiniature->getActor()->GetCenter();
	//m_modelInMiniature->getActor()->AddPosition(cPos[controllerID][0] - centerPos[0], cPos[controllerID][1] - centerPos[1] + offset, cPos[controllerID][2] - centerPos[2]);
	m_modelInMiniature->getActor()->SetPosition(cPos[controllerID][0], cPos[controllerID][1], cPos[controllerID][2]);

	calculateMetrics();
}

void iAVRMain::spawnModelInMiniature(double eventPosition[3], bool hide)
{
	if(!hide)
	{
		modelInMiniatureActive = true;
		updateModelInMiniatureData();
		onZoom(); //reset to current zoom
		m_modelInMiniature->show();

		fiberMetrics->moveColorBarLegend(eventPosition);
		fiberMetrics->showColorBarLegend();
	}
	else
	{
		modelInMiniatureActive = false;
		m_MIPPanelsVisible = false;
		m_modelInMiniature->hide();
		m_modelInMiniature->removeHighlightedGlyphs();
		m_volume->removeHighlightedGlyphs();
		fiberMetrics->hideColorBarLegend();
		fiberMetrics->hideMIPPanels();
		m_slider->hide();
	}
}

void iAVRMain::pressLeftTouchpad()
{
	auto initWorldScale = m_vrEnv->getInitialWorldScale();
	double offsetMiM = initWorldScale * 0.0095;
	double offsetVol = initWorldScale * 0.034;
	iAVRTouchpadPosition touchpadPos = m_style->getTouchedPadSide(touchPadPosition);

	if (modelInMiniatureActive && currentOctreeLevel > 0)
	{
		m_MIPPanelsVisible = false;
		fiberMetrics->hideMIPPanels();


		if (touchpadPos == iAVRTouchpadPosition::Up || touchpadPos == iAVRTouchpadPosition::Down)
		{

			if (touchpadPos == iAVRTouchpadPosition::Up)
			{
				//Just use positive offset
			}
			if (touchpadPos == iAVRTouchpadPosition::Down)
			{
				offsetMiM = offsetMiM * -1;
				offsetVol = offsetVol * -1;
			}

			switch (currentMiMDisplacementType)
			{
			case -1:
				DEBUG_LOG(QString("Unknown Displacement!"));
				break;
			case 0:
				m_modelInMiniature->applyRelativeCubeOffset(offsetMiM);
				m_volume->applyRelativeCubeOffset(offsetVol);
				m_volume->moveFibersByMaxCoverage(fiberMetrics->getMaxCoverageFiberPerRegion(), offsetVol);
				break;
			case 1:
				m_modelInMiniature->applyLinearCubeOffset(offsetMiM);
				m_volume->applyLinearCubeOffset(offsetVol);
				m_volume->moveFibersByMaxCoverage(fiberMetrics->getMaxCoverageFiberPerRegion(), offsetVol);
				//m_volume->moveFibersbyAllCoveredRegions(offset);
				break;
			case 2:
				m_modelInMiniature->apply4RegionCubeOffset(offsetMiM);
				m_volume->apply4RegionCubeOffset(offsetVol);
				m_volume->moveFibersByMaxCoverage(fiberMetrics->getMaxCoverageFiberPerRegion(), offsetVol);
				break;
			}

		}
		else if (touchpadPos == iAVRTouchpadPosition::Right || touchpadPos == iAVRTouchpadPosition::Left)
		{
			if (touchpadPos == iAVRTouchpadPosition::Right)
			{
				m_volume->filterRegionLinks(1);
				m_slider->setValue(m_volume->getJaccardFilterVal());

			}
			if (touchpadPos == iAVRTouchpadPosition::Left)
			{

				m_volume->filterRegionLinks(-1);
				m_slider->setValue(m_volume->getJaccardFilterVal());
			}
		}

		if (m_networkGraphMode)	m_volume->createRegionLinks(fiberMetrics->getWeightedJaccardIndex(currentOctreeLevel), fiberMetrics->getMaxNumberOfFibersInRegion(currentOctreeLevel), initWorldScale);
	}
}

void iAVRMain::changeMiMDisplacementType()
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

void iAVRMain::rotateDistributionVis(double eventPosition[3], bool startAction)
{
	if(startAction)
	{
		//m_distributionVis->flipThroughHistograms(sign);
	}
	else
	{
		m_distributionVis->flipThroughHistograms(sign);
	}
}

void iAVRMain::displayNodeLinkD()
{

	if (!m_networkGraphMode)
	{
		m_volume->hide();
		m_volume->hideVolume();
		m_volume->createRegionLinks(fiberMetrics->getWeightedJaccardIndex(currentOctreeLevel), fiberMetrics->getMaxNumberOfFibersInRegion(currentOctreeLevel), m_vrEnv->getInitialWorldScale());
		m_volume->showRegionLinks();

		m_slider->createSlider(0.0, 1.0, QString("Jaccard Index").toUtf8());
		m_slider->setSliderLength(m_vrEnv->getInitialWorldScale() * 0.17);
		m_slider->show();

		m_networkGraphMode = true;
	}
	else
	{
		m_volume->show();
		m_volume->showVolume();
		m_distributionVis->hide();
		m_volume->hideRegionLinks();

		m_slider->hide();

		m_networkGraphMode = false;
	}
}