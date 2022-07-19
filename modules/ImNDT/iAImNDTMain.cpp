/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAImNDTMain.h"

#include "iA3DColoredPolyObjectVis.h"
#include "iAImNDTInteractorStyle.h"
#include "iAVR3DText.h"
#include "iAVRColorLegend.h"
#include "iAVRFrontCamera.h"
#include "iAVRHistogramPairVis.h"
#include "iAVRInteractor.h"
#include "iAVRMip.h"
#include "iAVRModelInMiniature.h"
#include "iAVRObjectModel.h"
#include "iAVROctree.h"
#include "iAVRSlider.h"
#include <iALog.h>

#include "vtkActor.h"
#include "vtkIntersectionPolyDataFilter.h"
#include "vtkLineSource.h"
#include "vtkOpenVRCamera.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkVertexGlyphFilter.h"

#include <QColor>

//Octree Max Level
#define OCTREE_MAX_LEVEL 3
#define OCTREE_MIN_LEVEL 0
#define OCTREE_POINTS_PER_REGION 1
#define OCTREE_COLOR QColor(126, 0, 223, 255)
//#define OCTREE_COLOR QColor(130, 10, 10, 255)

iAImNDTMain::iAImNDTMain(iAVREnvironment* vrEnv, iAImNDTInteractorStyle* style, iA3DColoredPolyObjectVis* polyObject, vtkTable* objectTable, iACsvIO io, iACsvConfig csvConfig) : m_vrEnv(vrEnv),
		m_style(style), m_polyObject(polyObject), m_objectTable(objectTable), m_io(io)
{

	// For true TranslucentGeometry
	//https://vtk.org/Wiki/VTK/Examples/Cxx/Visualization/CorrectlyRenderTranslucentGeometry#CorrectlyRenderTranslucentGeometry.cxx
	//m_vrEnv->renderer()->SetUseDepthPeeling(true);
	//m_vrEnv->renderer()->SetMaximumNumberOfPeels(2);
	//m_vrEnv->renderer()->SetUseFXAA(true);
	
	currentMiMDisplacementType = 0;

	//Initialize Cube Vis
	m_modelInMiniature = new iAVRModelInMiniature(m_vrEnv->renderer());
	m_volume = new iAVRObjectModel(m_vrEnv->renderer(), m_polyObject, m_objectTable, m_io, csvConfig);

	//Define Octree
	currentOctreeLevel = 0;
	m_octrees = new std::vector<iAVROctree*>();
	generateOctrees(OCTREE_MAX_LEVEL, OCTREE_POINTS_PER_REGION, m_volume->getPolyObject()->polyData());
	//m_octrees->at(currentOctreeLevel)->generateOctreeRepresentation(currentOctreeLevel, OCTREE_COLOR);
	//m_octrees->at(currentOctreeLevel)->show();

	//Set Octree
	m_modelInMiniature->setOctree(m_octrees->at(OCTREE_MIN_LEVEL));
	m_volume->setOctree(m_octrees->at(OCTREE_MIN_LEVEL));

	//Initialize Metrics class
	currentFeature = 1;
	fiberMetrics = new iAVROctreeMetrics(m_objectTable, m_io, m_octrees);
	histogramMetrics = new iAVRHistogramMetric(m_objectTable, m_io, m_octrees);

	//Fiber Coverage
	m_fiberCoverageCalc = new iAVRObjectCoverage(m_objectTable, m_io, csvConfig, m_octrees, m_volume);
	m_fiberCoverageCalc->calculateObjectCoverage();


	m_volume->setFiberCoverageData(m_fiberCoverageCalc->getObjectCoverage());
	m_modelInMiniature->setFiberCoverageData(m_fiberCoverageCalc->getObjectCoverage());
	fiberMetrics->setFiberCoverageData(m_fiberCoverageCalc->getObjectCoverage());
	histogramMetrics->setFiberCoverageData(m_fiberCoverageCalc->getObjectCoverage());
	fiberMetrics->getMaxCoverageFiberPerRegion();

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

	//Initialize ColorLegend
	m_MiMColorLegend = new iAVRColorLegend(m_vrEnv->renderer());

	//Initialize ColorLegend
	m_MiMMip = new iAVRMip(m_vrEnv->renderer(), m_octrees, m_MiMColorLegend);

	//Initialize Slider
	m_slider = new iAVRSlider(m_vrEnv->renderer(), m_vrEnv->interactor());

	//Initialize Distribution Vis
	m_networkGraphMode = false;
	m_distributionVis = new iAVRHistogramPairVis(m_vrEnv->renderer(),histogramMetrics, fiberMetrics, m_objectTable, m_io);

	//Display Volume
	m_volume->createCubeModel();
	m_volume->showVolume();
	m_volume->show();

	//Add Input Mapping
	//Press, Touch
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::Volume, iAVROperations::PickFibersinRegion);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::MiniatureModel, iAVROperations::PickMiMRegion);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::ResetSelection);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::Histogram, iAVROperations::FlipHistoBookPages);
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
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Grip, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::LeftGrid);

	//Release, Untouch
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Grip, vtkEventDataAction::Release,
		iAVRInteractionOptions::Anywhere, iAVROperations::MultiPickMiMRegion);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Grip, vtkEventDataAction::Release,
		iAVRInteractionOptions::Anywhere, iAVROperations::LeftGrid);

	//For Oculus
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Joystick, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeOctreeAndMetric);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Joystick, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ExplodeMiM);
	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Joystick, vtkEventDataAction::Press,
		iAVRInteractionOptions::Anywhere, iAVROperations::ChangeJaccardIndex);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Release,
		iAVRInteractionOptions::Histogram, iAVROperations::FlipHistoBookPages);

	//Add Actors
	addPropToOptionID(vtkProp3D::SafeDownCast(m_modelInMiniature->getActor()), iAVRInteractionOptions::MiniatureModel);
	addPropToOptionID(vtkProp3D::SafeDownCast(m_volume->getActor()), iAVRInteractionOptions::Volume);
	addPropToOptionID(vtkProp3D::SafeDownCast(m_volume->getVolumeActor()), iAVRInteractionOptions::Volume);

	//addPropToOptionID(m_slider->getSlider(), iAVRInteractionOptions::Histogram);
	
	for(size_t i = 0; i < m_octrees->size(); i++)
	{
		addPropToOptionID(vtkProp3D::SafeDownCast(m_octrees->at(i)->getActor()), iAVRInteractionOptions::Volume); //Octree counts as Volume
	}
}

//! Defines the action executed for specific controller inputs
//! Position and Orientation are in WorldCoordinates and Orientation is in Degree
void iAImNDTMain::startInteraction(vtkEventDataDevice3D* device, vtkProp3D* pickedProp, double eventPosition[3], double eventOrientation[4])
{
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 1, 0)
	auto touchPos = m_style->getTrackPadPos(device->GetDevice());
	m_touchPadPosition[0] = touchPos.c[0];
	m_touchPadPosition[1] = touchPos.c[1];
	m_touchPadPosition[2] = 0.0;
#else
	m_vrEnv->interactor()->GetTouchPadPosition(device->GetDevice(), device->GetInput(), m_touchPadPosition);
#endif

	int deviceID = static_cast<int>(device->GetDevice()); // Device
	int inputID = static_cast<int>(device->GetInput());  // Input Method
	int actioniD = static_cast<int>(device->GetAction()); // Action of Input Method
	int optionID = getOptionForObject(pickedProp);
	int operation = m_style->getInputScheme()->at(deviceID).at(inputID).at(actioniD).at(optionID);

	switch (iAVROperations(operation))
	{
	case iAVROperations::Unknown:
		LOG(lvlDebug,QString("Unknown Operation!"));
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
		if (activeInput->at(deviceID) == 1)
		{
			this->toggleArView();
		}
		else
		{
			this->changeMiMDisplacementType();
		}
		break;
	case iAVROperations::DisplayNodeLinkDiagram:
		this->displayNodeLinkD();
		break;
	case iAVROperations::ChangeJaccardIndex:
		this->pressLeftTouchpad();
		break;
	case iAVROperations::FlipHistoBookPages:
		activeInput->at(deviceID) = static_cast<int>(iAVROperations::FlipHistoBookPages);
		this->flipDistributionVis();
		break;
	case iAVROperations::LeftGrid:
		activeInput->at(deviceID) = 1;
		break;
	default:
		LOG(lvlDebug, QString("Start Operation %1 is not defined").arg(operation));
		break;
	}
}

void iAImNDTMain::endInteraction(vtkEventDataDevice3D* device, vtkProp3D* pickedProp, double eventPosition[3], double eventOrientation[4])
{
	Q_UNUSED(eventPosition);
	Q_UNUSED(eventOrientation);

	int deviceID = static_cast<int>(device->GetDevice()); // Device
	int inputID = static_cast<int>(device->GetInput());  // Input Method
	int actioniD = static_cast<int>(device->GetAction()); // Action of Input Method
	int optionID = getOptionForObject(pickedProp);
	int operation = m_style->getInputScheme()->at(deviceID).at(inputID).at(actioniD).at(optionID);

	switch (iAVROperations(operation))
	{
	case iAVROperations::Unknown:
		LOG(lvlDebug,QString("Unknown Operation!"));
		break;
	case iAVROperations::None:
		//activeInput->at(deviceID) = static_cast<int>(iAVROperations::None); // For Multitouch
		break;
	case iAVROperations::MultiPickMiMRegion:
		activeInput->at(deviceID) = 0; // For Multitouch
		multiPickMiMRegion();
		break;
	case iAVROperations::FlipHistoBookPages:
		this->flipDistributionVis();
		activeInput->at(deviceID) = 0;
		break;
	case iAVROperations::LeftGrid:
		activeInput->at(deviceID) = 0;
		break;
	default:
		LOG(lvlDebug, QString("End Operation %1 is not defined").arg(operation));
		break;
	}
	m_vrEnv->update();
}

void iAImNDTMain::onMove(vtkEventDataDevice3D * device, double movePosition[3], double eventOrientation[4])
{
	vtkSmartPointer<vtkCamera> cam = m_vrEnv->renderer()->GetActiveCamera();
	double initialScale = m_vrEnv->interactor()->GetPhysicalScale();
	//Currently moved controller
	int deviceID = static_cast<int>(device->GetDevice());

	//TODO Initialize only after first call or they are filled wrong!
	int oldViewDirection = viewDirection;
	//double oldFocalPoint[3] = {focalPoint[0], focalPoint[1], focalPoint[2]};
	double oldcPos[3] = { cPos[deviceID][0], cPos[deviceID][1], cPos[deviceID][2]};
	//double oldcOrie[4] = { cOrie[deviceID][0], cOrie[deviceID][1], cOrie[deviceID][2], cOrie[deviceID][3]}; //W,X,Y,Z

	for (vtkIdType i = 0; i < 3; i++)
	{
		//Save Current Pos
		cPos[deviceID][i] = movePosition[i];

		//Save Current Pos
		cOrie[deviceID][i] = eventOrientation[i];
	}
	cOrie[deviceID][3] = eventOrientation[3];

	//Movement of Head
	if (deviceID == static_cast<int>(vtkEventDataDevice::HeadMountedDisplay))
	{
		if (arEnabled)
		{
			arViewer->refreshImage();
		}

		double* tempFocalPos = cam->GetFocalPoint();
		focalPoint[0] = tempFocalPos[0];
		focalPoint[1] = tempFocalPos[1];
		focalPoint[2] = tempFocalPos[2];

		m_3DTextLabels->at(2)->setLabelPos(tempFocalPos);

		double* tempViewDirection = cam->GetDirectionOfProjection();
		viewDirection = static_cast<int>(m_style->getViewDirection(tempViewDirection));

		if (m_networkGraphMode)
		{
			//Calc half distance of focalPoint vec
			iAVec3d focalP = iAVec3d(tempFocalPos);
			iAVec3d viewDir = iAVec3d(tempViewDirection);
			viewDir = viewDir * (cam->GetDistance() / 1.5);
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
					m_MiMMip->createSingleMIPPanel(currentOctreeLevel, currentFeature, viewDirection, m_vrEnv->getInitialWorldScale(), fiberMetrics->getRegionAverage(currentOctreeLevel, currentFeature));
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

			m_MiMColorLegend->setPosition(colorLegendlcPos);
			//Rotate legend around y
			m_MiMColorLegend->setOrientation(0, -cam->GetOrientation()[1], 0);

			m_MiMColorLegend->setTitle(QString(" %1 ").arg(fiberMetrics->getFeatureName(currentFeature)).toUtf8());
			m_MiMColorLegend->show();
		}

		double lcPos[3] = { cPos[deviceID][0], cPos[deviceID][1] - (initialScale * 0.03), cPos[deviceID][2]};
		m_3DTextLabels->at(1)->setLabelPos(lcPos);
		m_3DTextLabels->at(1)->moveInEyeDir(initialScale * 0.04, initialScale * 0.04, initialScale * 0.04);

		m_slider->setPosition(lcPos[0], lcPos[1] - (initialScale * 0.04), lcPos[2] + (initialScale * 0.07));
	}

	//Movement of Right controller
	if (deviceID == static_cast<int>(vtkEventDataDevice::RightController))
	{
		double rcPos[3] = { cPos[deviceID][0], cPos[deviceID][1] - (initialScale * 0.03), cPos[deviceID][2]};
		m_3DTextLabels->at(0)->setLabelPos(rcPos);
		m_3DTextLabels->at(0)->moveInEyeDir(initialScale * 0.04, initialScale * 0.04, initialScale * 0.04);

		if(activeInput->at(deviceID) == static_cast<int>(iAVROperations::FlipHistoBookPages))
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

//! Corrects the size of elements based on the new physical scale of the environment
void iAImNDTMain::onZoom()
{
	auto scaleDiff = (1.0 / calculateWorldScaleFactor());

	m_modelInMiniature->setScale(scaleDiff, scaleDiff, scaleDiff);
	m_MiMColorLegend->setScale(scaleDiff);
}

void iAImNDTMain::setInputScheme(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, iAVRInteractionOptions options, iAVROperations operation)
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
int iAImNDTMain::getOptionForObject(vtkProp3D* pickedProp)
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

void iAImNDTMain::addPropToOptionID(vtkProp3D* prop, iAVRInteractionOptions iD)
{
	m_ActorToOptionID.insert(std::make_pair(prop, static_cast<int>(iD)));
}

//! Test method inserts colored point at given Position
void iAImNDTMain::drawPoint(std::vector<double*>* pos, QColor color)
{
	m_vrEnv->renderer()->RemoveActor(pointsActor);

	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

	for(size_t i=0; i<pos->size(); i++)
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

//! Generates octrees until maxLevel is reached or no more leafNodes are added through later levels
void iAImNDTMain::generateOctrees(int maxLevel, int maxPointsPerRegion, vtkPolyData* dataSet)
{
	int lastLeafNodeAmount = 0;
	for(auto level = 0; level <= maxLevel; level++)
	{
		iAVROctree* tempOctree = new iAVROctree(m_vrEnv->renderer(), dataSet);
		tempOctree->calculateOctree(level, maxPointsPerRegion);
		// Check if leafNodes have been added, if not stop creating deeper levels
		if (tempOctree->getNumberOfLeafeNodes() <= lastLeafNodeAmount) break;
		lastLeafNodeAmount = tempOctree->getNumberOfLeafeNodes();

		m_octrees->push_back(tempOctree);
	}
	if(m_octrees->empty()) LOG(lvlDebug, QString("NO OCTREE GENERATED"));
}

void iAImNDTMain::calculateMetrics()
{

	m_MiMColorLegend->hide();
	m_MiMMip->hideMIPPanels();

	// NEW //
	auto minMax = fiberMetrics->getMinMaxAvgRegionValues(currentOctreeLevel, currentFeature);
	m_MiMColorLegend->createLut(minMax.at(0), minMax.at(1), 1);
	std::vector<QColor>* rgba = m_MiMColorLegend->getColors(currentOctreeLevel, currentFeature, fiberMetrics->getRegionAverage(currentOctreeLevel, currentFeature));
	m_MiMColorLegend->calculateLegend(m_vrEnv->getInitialWorldScale());

	if (modelInMiniatureActive) 
	{
		m_modelInMiniature->applyHeatmapColoring(rgba); // Only call when model is calculated (poly data has to be accessible)

		m_MiMColorLegend->setTitle(QString(" %1 ").arg(fiberMetrics->getFeatureName(currentFeature)).toUtf8());

		m_MIPPanelsVisible = true;
		m_MiMMip->createSingleMIPPanel(currentOctreeLevel, currentFeature,viewDirection, m_vrEnv->getInitialWorldScale(), fiberMetrics->getRegionAverage(currentOctreeLevel, currentFeature));
	}
	
}

void iAImNDTMain::colorMiMCubes(std::vector<vtkIdType>* regionIDs)
{

	std::vector<QColor>* rgba = m_MiMColorLegend->getColors(currentOctreeLevel, currentFeature, fiberMetrics->getRegionAverage(currentOctreeLevel, currentFeature));
	m_MiMColorLegend->calculateLegend(m_vrEnv->getInitialWorldScale());
	m_modelInMiniature->applyHeatmapColoring(rgba); //Reset Color
	
	for (size_t i = 0; i < regionIDs->size(); i++)
	{
		m_modelInMiniature->setCubeColor(OCTREE_COLOR, regionIDs->at(i));
	}
	
}

//! Returns the difference between the intial world scaling and the current scaling
double iAImNDTMain::calculateWorldScaleFactor()
{
	double currentScale = m_vrEnv->interactor()->GetPhysicalScale();
	auto temp = m_vrEnv->getInitialWorldScale()/ currentScale;

	return temp;
}

//! Increases/Decreases the current octree level and feature. Recalculates the Model in Miniature Object.
void iAImNDTMain::changeOctreeAndMetric()
{
	iAVRTouchpadPosition touchpadPos = iAImNDTInteractorStyle::getTouchedPadSide(m_touchPadPosition);

	if (touchpadPos == iAVRTouchpadPosition::Up || touchpadPos == iAVRTouchpadPosition::Down) {

		//m_octrees->at(currentOctreeLevel)->hide(); // Hide old octree
		m_volume->hide();
		m_volume->hideRegionLinks();
		m_volume->hideVolume();
		m_volume->resetVolume();
		addPropToOptionID(vtkProp3D::SafeDownCast(m_volume->getVolumeActor()), iAVRInteractionOptions::Volume);

		if (touchpadPos == iAVRTouchpadPosition::Up && currentOctreeLevel < static_cast<int>(m_octrees->size()) - 1)
		{
			currentOctreeLevel++;
		}
		if (touchpadPos == iAVRTouchpadPosition::Down && currentOctreeLevel > OCTREE_MIN_LEVEL)
		{
			currentOctreeLevel--;
		}

		QString text = QString("Octree Level %1").arg(currentOctreeLevel);
		m_3DTextLabels->at(0)->create3DLabel(text);
		m_3DTextLabels->at(0)->show();
		m_3DTextLabels->at(1)->hide();

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

		m_3DTextLabels->at(0)->hide();
	}
	updateModelInMiniatureData();
}

void iAImNDTMain::pickSingleFiber(double eventPosition[3])
{
	std::vector<size_t> selection = std::vector<size_t>();
	// Find the closest points to TestPoint
	vtkIdType iD = m_octrees->at(currentOctreeLevel)->getOctree()->FindClosestPoint(eventPosition);

	vtkIdType rowiD = m_fiberCoverageCalc->getObjectiD(iD);
	selection.push_back(rowiD);

	m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
	emit selectionChanged();
}

//! Picks all fibers in the region clicked by the user.
void iAImNDTMain::pickFibersinRegion(double eventPosition[3], double eventOrientation[4])
{
	vtkIdType cellID = m_volume->getClosestCellID(eventPosition, eventOrientation);

	if (cellID >= 0 && cellID <= m_octrees->at(currentOctreeLevel)->getNumberOfLeafeNodes())
	{
		pickFibersinRegion(cellID);
	}
}

//! Picks all fibers in the octree region defined by the leaf node ID.
void iAImNDTMain::pickFibersinRegion(int leafRegion)
{
	std::vector<size_t> selection = std::vector<size_t>();

	
	for (auto fiber : *m_fiberCoverageCalc->getObjectCoverage()->at(currentOctreeLevel).at(leafRegion)) {
		//LOG(lvlImportant,QString("Nr. [%1]").arg(fiber.first));
		selection.push_back(fiber.first);
	}
	std::sort(selection.begin(), selection.end());

	multiPickIDs->push_back(leafRegion);
	if(modelInMiniatureActive) m_modelInMiniature->highlightGlyphs(multiPickIDs);
	m_volume->highlightGlyphs(multiPickIDs);

	multiPickIDs->clear();
	m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
	emit selectionChanged();
}

void iAImNDTMain::pickMimRegion(double eventPosition[3], double eventOrientation[4])
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
void iAImNDTMain::multiPickMiMRegion()
{	
	if(!multiPickIDs->empty())
	{
		std::vector<size_t> selection = std::vector<size_t>();

		for (size_t i = 0; i < multiPickIDs->size(); i++)
		{
			for (auto fiber : *m_fiberCoverageCalc->getObjectCoverage()->at(currentOctreeLevel).at(multiPickIDs->at(i))) {
				selection.push_back(fiber.first);
			}
		}

		std::sort(selection.begin(), selection.end());
		selection.erase(std::unique(selection.begin(), selection.end()), selection.end());

		//If not Network Mode then render fibers
		if (!m_networkGraphMode)
		{
			m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
			emit selectionChanged();
		}
		else
		{
			m_distributionVis->hide();
			multiPickIDs->resize(2); //only two regions allowed

			std::vector<int> featureList;
			//featureList->push_back(currentFeature); //
			featureList.push_back(21);
			featureList.push_back(20);
			featureList.push_back(19);
			featureList.push_back(11);
			featureList.push_back(18);
			featureList.push_back(17);
			featureList.push_back(7);

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

void iAImNDTMain::resetSelection()
{
	m_volume->renderSelection(std::vector<size_t>(), 0, QColor(140, 140, 140, 255), nullptr);
	emit selectionChanged();
	m_volume->removeHighlightedGlyphs();
	if(modelInMiniatureActive)
	{
		std::vector<QColor>* rgba = m_MiMColorLegend->getColors(currentOctreeLevel, currentFeature, fiberMetrics->getRegionAverage(currentOctreeLevel, currentFeature));
		m_MiMColorLegend->calculateLegend(m_vrEnv->getInitialWorldScale());
		onZoom();
		m_modelInMiniature->applyHeatmapColoring(rgba); //Reset Color
		m_volume->resetNodeColor();
		m_modelInMiniature->removeHighlightedGlyphs();
		//fiberMetrics->hideMIPPanels();
	}
	m_distributionVis->hide();
}

//! Updates the data (Octree, Metrics,...) and the position for the current MiM
void iAImNDTMain::updateModelInMiniatureData()
{
	int controllerID = static_cast<int>(vtkEventDataDevice::LeftController);
	
	m_modelInMiniature->setOctree(m_octrees->at(currentOctreeLevel));
	m_modelInMiniature->createCubeModel(); //Here a new MiM is calculated
	
	//m_networkGraphMode = false;
	resetSelection();

	m_modelInMiniature->getActor()->SetPosition(cPos[controllerID][0], cPos[controllerID][1], cPos[controllerID][2]);

	calculateMetrics();
	onZoom();
}

void iAImNDTMain::spawnModelInMiniature(double eventPosition[3], bool hide)
{
	if(!hide)
	{
		modelInMiniatureActive = true;
		updateModelInMiniatureData();
		onZoom(); //reset to current zoom
		m_modelInMiniature->show();
	
		m_MiMColorLegend->setPosition(eventPosition);
		m_MiMColorLegend->show();
	}
	else
	{
		modelInMiniatureActive = false;
		m_MIPPanelsVisible = false;
		m_modelInMiniature->hide();
		m_modelInMiniature->removeHighlightedGlyphs();
		m_volume->removeHighlightedGlyphs();
		m_MiMColorLegend->hide();
		m_MiMMip->hideMIPPanels();
		m_slider->hide();
	}
}

void iAImNDTMain::pressLeftTouchpad()
{
	auto initWorldScale = m_vrEnv->getInitialWorldScale();
	double offsetMiM = initWorldScale * 0.022;
	double offsetVol = initWorldScale * 0.039;
	iAVRTouchpadPosition touchpadPos = iAImNDTInteractorStyle::getTouchedPadSide(m_touchPadPosition);

	if (modelInMiniatureActive && currentOctreeLevel > 0)
	{
		m_MIPPanelsVisible = false;
		m_MiMMip->hideMIPPanels();


		if (touchpadPos == iAVRTouchpadPosition::Up || touchpadPos == iAVRTouchpadPosition::Down)
		{
			m_3DTextLabels->at(1)->show();

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
				LOG(lvlDebug, QString("Unknown Displacement!"));
				break;
			case 0:
				m_modelInMiniature->applySPDisplacement(offsetMiM);
				m_volume->applySPDisplacement(offsetVol);
				m_volume->moveFibersByMaxCoverage(fiberMetrics->getMaxCoverageFiberPerRegion(), offsetVol, true);
				break;
			case 1:
				m_modelInMiniature->applyRadialDisplacement(offsetMiM);
				m_volume->applyRadialDisplacement(offsetVol);
				m_volume->moveFibersByMaxCoverage(fiberMetrics->getMaxCoverageFiberPerRegion(), offsetVol, false);
				break;
			case 2:
				m_modelInMiniature->applyOctantDisplacement(offsetMiM);
				m_volume->applyOctantDisplacement(offsetVol);
				m_volume->moveFibersbyOctant(fiberMetrics->getMaxCoverageFiberPerRegion(), offsetVol);
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

		if (m_networkGraphMode)	m_volume->createSimilarityNetwork(fiberMetrics->getJaccardIndex(currentOctreeLevel), fiberMetrics->getMaxNumberOfFibersInRegion(currentOctreeLevel), initWorldScale);
	}
}

void iAImNDTMain::changeMiMDisplacementType()
{
	currentMiMDisplacementType++;

	if(currentMiMDisplacementType > 2)
	{
		currentMiMDisplacementType = 0;
	}

	switch (currentMiMDisplacementType)
	{
	case 0:
		m_3DTextLabels->at(1)->create3DLabel(QString("SP displacement"));
		break;
	case 1:
		m_3DTextLabels->at(1)->create3DLabel(QString("Radial displacement"));
		break;
	case 2:
		m_3DTextLabels->at(1)->create3DLabel(QString("Octant displacement"));
		break;
	}

	m_3DTextLabels->at(1)->show();
	m_3DTextLabels->at(0)->hide();	
}

void iAImNDTMain::flipDistributionVis()
{
	m_distributionVis->flipThroughHistograms(sign);
}

void iAImNDTMain::displayNodeLinkD()
{

	if (!m_networkGraphMode)
	{
		m_volume->hide();
		m_volume->hideVolume();
		m_volume->createSimilarityNetwork(fiberMetrics->getJaccardIndex(currentOctreeLevel), fiberMetrics->getMaxNumberOfFibersInRegion(currentOctreeLevel), m_vrEnv->getInitialWorldScale());
		m_volume->showRegionLinks();

		m_slider->createSlider(0.0, 1.0, QString("Jaccard Index").toUtf8());
		m_slider->setSliderLength(m_vrEnv->getInitialWorldScale() * 0.17);
		m_slider->setValue(m_volume->getJaccardFilterVal());
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

void iAImNDTMain::toggleArView()
{
	if (!arEnabled)
	{
		m_vrEnv->hideSkybox();
		arViewer = new iAVRFrontCamera(m_vrEnv->renderer(), m_vrEnv->renderWindow());
		arViewer->initialize();
		arViewer->buildRepresentation();
		arViewer->refreshImage();
		arEnabled = true;
		return;
	}

	arEnabled = false;
	m_vrEnv->showSkybox();
	
}
