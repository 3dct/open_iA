// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImNDTMain.h"

#include "iAImNDTInteractorStyle.h"
#include "iAVR3DText.h"
#include "iAVRColorLegend.h"
#include "iAVREnvironment.h"
#include "iAVRFrontCamera.h"
#include "iAVRHistogramMetric.h"
#include "iAVRHistogramPairVis.h"
#include "iAVRMip.h"
#include "iAVRModelInMiniature.h"
#include "iAVRObjectCoverage.h"
#include "iAVRObjectModel.h"
#include "iAVROctree.h"
#include "iAVROctreeMetrics.h"
#include "iAVRSlider.h"

#include <iAColoredPolyObjectVis.h>
#include <iAObjectsData.h>

#include <iALog.h>

#include <vtkAssembly.h>
#include <vtkCamera.h>
#include <vtkInteractorStyle3D.h>
#include <vtkOctreePointLocator.h>
#include <vtkPolyData.h>
#include <vtkProp3D.h>
#include <vtkTable.h>

#include <QColor>

//Octree Max Level
#define OCTREE_MAX_LEVEL 3
#define OCTREE_MIN_LEVEL 0
#define OCTREE_POINTS_PER_REGION 1
#define OCTREE_COLOR QColor(126, 0, 223, 255)
//#define OCTREE_COLOR QColor(130, 10, 10, 255)

iAImNDTMain::iAImNDTMain(iAVREnvironment* vrEnv, iAColoredPolyObjectVis* polyObject, iAObjectsData const * objData, iACsvConfig csvConfig) :
	m_vrEnv(vrEnv), m_interactions(vrEnv->backend(), this), m_polyObject(polyObject),
	m_inputScheme(
		static_cast<size_t>(vtkEventDataDevice::NumberOfDevices),
		std::vector<std::vector<std::vector<int>>>(static_cast<size_t>(vtkEventDataDeviceInput::NumberOfInputs),
			std::vector<std::vector<int>>(static_cast<size_t>(vtkEventDataAction::NumberOfActions),
				std::vector<int>(static_cast<size_t>(iAVRInteractionOptions::NumberOfInteractionOptions), 0))
			)),
	m_activeInput(static_cast<size_t>(vtkEventDataDevice::NumberOfDevices), -1),
	m_arViewer(createARViewer(vrEnv))
{
	// For true TranslucentGeometry
	//https://vtk.org/Wiki/VTK/Examples/Cxx/Visualization/CorrectlyRenderTranslucentGeometry#CorrectlyRenderTranslucentGeometry.cxx
	//m_vrEnv->renderer()->SetUseDepthPeeling(true);
	//m_vrEnv->renderer()->SetMaximumNumberOfPeels(2);
	//m_vrEnv->renderer()->SetUseFXAA(true);
	
	m_currentMiMDisplacementType = 0;

	//Initialize Cube Vis
	m_modelInMiniature = new iAVRModelInMiniature(m_vrEnv->renderer());
	m_volume = new iAVRObjectModel(m_vrEnv->renderer(), m_polyObject);

	//Define Octree
	m_currentOctreeLevel = 0;
	generateOctrees(OCTREE_MAX_LEVEL, OCTREE_POINTS_PER_REGION, m_volume->getPolyObject()->polyData());
	//m_octrees.at(currentOctreeLevel)->generateOctreeRepresentation(currentOctreeLevel, OCTREE_COLOR);
	//m_octrees.at(currentOctreeLevel)->show();

	//Set Octree
	m_modelInMiniature->setOctree(m_octrees.at(OCTREE_MIN_LEVEL));
	m_volume->setOctree(m_octrees.at(OCTREE_MIN_LEVEL));

	//Initialize Metrics class
	m_currentFeature = 1;
	m_fiberMetrics = new iAVROctreeMetrics(objData->m_table, m_octrees);
	m_histogramMetrics = new iAVRHistogramMetric(objData->m_table, m_octrees);

	//Fiber Coverage
	m_fiberCoverageCalc = new iAVRObjectCoverage(objData->m_table, objData->m_colMapping, csvConfig, m_octrees, m_volume);
	m_fiberCoverageCalc->calculateObjectCoverage();

	m_volume->setFiberCoverageData(m_fiberCoverageCalc->getObjectCoverage());
	m_modelInMiniature->setFiberCoverageData(m_fiberCoverageCalc->getObjectCoverage());
	m_fiberMetrics->setFiberCoverageData(m_fiberCoverageCalc->getObjectCoverage());
	m_histogramMetrics->setFiberCoverageData(m_fiberCoverageCalc->getObjectCoverage());
	m_fiberMetrics->getMaxCoverageFiberPerRegion();

	//Initialize Text Lables vector
	m_3DTextLabels.push_back(new iAVR3DText(m_vrEnv->renderer()));// [0] for Octree level change
	m_3DTextLabels.push_back(new iAVR3DText(m_vrEnv->renderer()));// [1] for feature change
	m_3DTextLabels.push_back(new iAVR3DText(m_vrEnv->renderer()));// [2] for Alerts

	//Initialize ColorLegend
	m_MiMColorLegend = new iAVRColorLegend(m_vrEnv->renderer());

	//Initialize Maximum Intensity Projection
	m_MiMMip = new iAVRMip(m_vrEnv->renderer(), m_octrees, m_MiMColorLegend);

	//Initialize Slider
	m_slider = new iAVRSlider(m_vrEnv->renderer(), m_vrEnv->interactor());

	//Initialize Distribution Vis
	m_networkGraphMode = false;
	m_distributionVis = new iAVRHistogramPairVis(m_vrEnv->renderer(), m_histogramMetrics, m_fiberMetrics, objData->m_table);

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
	
	for(size_t i = 0; i < m_octrees.size(); i++)
	{
		addPropToOptionID(vtkProp3D::SafeDownCast(m_octrees.at(i)->getActor()), iAVRInteractionOptions::Volume); //Octree counts as Volume
	}

	//Add InteractorStyle
	m_vrEnv->interactor()->SetInteractorStyle(m_interactions.style());
}

iAImNDTMain::~iAImNDTMain() =default;

void iAImNDTMain::startInteraction(vtkEventDataDevice3D* device, vtkProp3D* pickedProp, double eventPosition[3], double eventOrientation[4])
{
	auto touchPos = m_interactions.getTrackPadPos(device->GetDevice());
	m_touchPadPosition[0] = touchPos.c[0];
	m_touchPadPosition[1] = touchPos.c[1];
	m_touchPadPosition[2] = 0.0;
	int deviceID = static_cast<int>(device->GetDevice()); // Device
	int inputID = static_cast<int>(device->GetInput());  // Input Method
	int actioniD = static_cast<int>(device->GetAction()); // Action of Input Method
	int optionID = getOptionForObject(pickedProp);
	int operation = m_inputScheme.at(deviceID).at(inputID).at(actioniD).at(optionID);

	switch (iAVROperations(operation))
	{
	case iAVROperations::Unknown:
		LOG(lvlDebug,QString("Unknown Operation!"));
		break;
	case iAVROperations::None:
		break;
	case iAVROperations::SpawnModelInMiniature:
		this->spawnModelInMiniature(eventPosition, m_modelInMiniatureActive);
		break;
	case iAVROperations::PickFibersinRegion:
		this->pickFibersinRegion(eventPosition, eventOrientation);
		break;
	case iAVROperations::PickMiMRegion:
		this->pickMimRegion(eventPosition, eventOrientation);
		break;
	case iAVROperations::MultiPickMiMRegion:	
		m_activeInput.at(deviceID) = static_cast<int>(iAVROperations::MultiPickMiMRegion); // For Multitouch
		//m_modelInMiniature->removeHighlightedGlyphs();
		m_multiPickIDs.clear();
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
		if (m_activeInput.at(deviceID) == 1)
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
		m_activeInput.at(deviceID) = static_cast<int>(iAVROperations::FlipHistoBookPages);
		this->flipDistributionVis();
		break;
	case iAVROperations::LeftGrid:
		m_activeInput.at(deviceID) = 1;
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
	int operation = m_inputScheme.at(deviceID).at(inputID).at(actioniD).at(optionID);

	switch (iAVROperations(operation))
	{
	case iAVROperations::Unknown:
		LOG(lvlDebug,QString("Unknown Operation!"));
		break;
	case iAVROperations::None:
		//m_activeInput.at(deviceID) = static_cast<int>(iAVROperations::None); // For Multitouch
		break;
	case iAVROperations::MultiPickMiMRegion:
		m_activeInput.at(deviceID) = 0; // For Multitouch
		multiPickMiMRegion();
		break;
	case iAVROperations::FlipHistoBookPages:
		this->flipDistributionVis();
		m_activeInput.at(deviceID) = 0;
		break;
	case iAVROperations::LeftGrid:
		m_activeInput.at(deviceID) = 0;
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
	int oldViewDirection = m_viewDirection;
	//double oldFocalPoint[3] = {focalPoint[0], focalPoint[1], focalPoint[2]};
	double oldcPos[3] = { m_cPos[deviceID][0], m_cPos[deviceID][1], m_cPos[deviceID][2]};
	//double oldcOrie[4] = { cOrie[deviceID][0], cOrie[deviceID][1], cOrie[deviceID][2], cOrie[deviceID][3]}; //W,X,Y,Z

	for (vtkIdType i = 0; i < 3; i++)
	{
		//Save Current Pos
		m_cPos[deviceID][i] = movePosition[i];

		//Save Current Pos
		m_cOrie[deviceID][i] = eventOrientation[i];
	}
	m_cOrie[deviceID][3] = eventOrientation[3];

	//Movement of Head
	if (deviceID == static_cast<int>(vtkEventDataDevice::HeadMountedDisplay))
	{
		if (m_arEnabled)
		{
			m_arViewer->refreshImage();
		}

		double* tempFocalPos = cam->GetFocalPoint();
		m_focalPoint[0] = tempFocalPos[0];
		m_focalPoint[1] = tempFocalPos[1];
		m_focalPoint[2] = tempFocalPos[2];

		m_3DTextLabels.at(2)->setLabelPos(tempFocalPos);

		double* tempViewDirection = cam->GetDirectionOfProjection();
		m_viewDirection = static_cast<int>(iAImNDTInteractions::getViewDirection(tempViewDirection));

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
		if(oldViewDirection != m_viewDirection)
		{
			if (m_modelInMiniatureActive)
			{
				if(m_MIPPanelsVisible)
				{
					m_MiMMip->createSingleMIPPanel(m_currentOctreeLevel, m_currentFeature, m_viewDirection, m_vrEnv->getInitialWorldScale(), m_fiberMetrics->getRegionAverage(m_currentOctreeLevel, m_currentFeature));
				}
			}
		}
	}

	//Movement of Left controller
	if(deviceID == static_cast<int>(vtkEventDataDevice::LeftController))
	{
		//For active Model in Minature MiM
		if(m_modelInMiniatureActive)
		{
			m_modelInMiniature->setPos(m_cPos[deviceID][0] - (initialScale * 0.08), m_cPos[deviceID][1] + (initialScale * 0.075), m_cPos[deviceID][2] - (initialScale * 0.085));

			double colorLegendlcPos[3] = { m_cPos[deviceID][0] + (initialScale * 0.04), m_cPos[deviceID][1] - (initialScale * 0.2), m_cPos[deviceID][2]};

			m_MiMColorLegend->setPosition(colorLegendlcPos);
			//Rotate legend around y
			m_MiMColorLegend->setOrientation(0, -cam->GetOrientation()[1], 0);

			m_MiMColorLegend->setTitle(QString(" %1 ").arg(m_fiberMetrics->getFeatureName(m_currentFeature)).toUtf8());
			m_MiMColorLegend->show();
		}

		double lcPos[3] = { m_cPos[deviceID][0], m_cPos[deviceID][1] - (initialScale * 0.03), m_cPos[deviceID][2]};
		m_3DTextLabels.at(1)->setLabelPos(lcPos);
		m_3DTextLabels.at(1)->moveInEyeDir(initialScale * 0.04, initialScale * 0.04, initialScale * 0.04);

		m_slider->setPosition(lcPos[0], lcPos[1] - (initialScale * 0.04), lcPos[2] + (initialScale * 0.07));
	}

	//Movement of Right controller
	if (deviceID == static_cast<int>(vtkEventDataDevice::RightController))
	{
		double rcPos[3] = { m_cPos[deviceID][0], m_cPos[deviceID][1] - (initialScale * 0.03), m_cPos[deviceID][2]};
		m_3DTextLabels.at(0)->setLabelPos(rcPos);
		m_3DTextLabels.at(0)->moveInEyeDir(initialScale * 0.04, initialScale * 0.04, initialScale * 0.04);

		if(m_activeInput.at(deviceID) == static_cast<int>(iAVROperations::FlipHistoBookPages))
		{
			double controllerPath[3]{};
			double crossPro[3]{};
			vtkMath::Subtract(oldcPos, m_cPos[deviceID], controllerPath);
			vtkMath::Cross(oldcPos, m_cPos[deviceID], crossPro);
			iAVec3d length = iAVec3d(controllerPath);
			m_controllerTravelledDistance = length.length();
			m_rotationOfDisVis = vtkMath::DegreesFromRadians(2 * asin(m_controllerTravelledDistance / (2 * m_distributionVis->getRadius())));
			m_sign = crossPro[1] < 0 ? -1 : 1;
			m_rotationOfDisVis *= m_sign;
		}
	}
}

void iAImNDTMain::onZoom()
{
	auto scaleDiff = (1.0 / calculateWorldScaleFactor());
	m_modelInMiniature->setScale(scaleDiff, scaleDiff, scaleDiff);
	m_MiMColorLegend->setScale(scaleDiff);
}

void iAImNDTMain::stop()
{
	// restore interactor:
	m_vrEnv->interactor()->SetInteractorStyle(defaultVRinteractorStyle(m_vrEnv->backend()));

	// hide all renderings:
	m_modelInMiniature->hide();
	m_modelInMiniature->removeHighlightedGlyphs();
	m_volume->hide();
	m_volume->hideRegionLinks();
	m_volume->hideVolume();
	m_volume->removeHighlightedGlyphs();
	m_MiMMip->hideMIPPanels();
	m_MiMColorLegend->hide();
	m_slider->hide();
	m_distributionVis->hide();
	for (auto t: m_3DTextLabels)
	{
		t->hide();
	}
	emit finished();
}

void iAImNDTMain::setInputScheme(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, iAVRInteractionOptions options, iAVROperations operation)
{
	if(options == iAVRInteractionOptions::Anywhere) //Apply Operation for every Interaction Option
	{
		for(int i=0; i < static_cast<int>(iAVRInteractionOptions::NumberOfInteractionOptions); i++)
		{
			m_inputScheme.at(static_cast<int>(device)).at(static_cast<int>(input)).at(static_cast<int>(action)).at(static_cast<int>(i)) = static_cast<int>(operation);
		}
	}
	else
	{
		m_inputScheme.at(static_cast<int>(device)).at(static_cast<int>(input)).at(static_cast<int>(action)).at(static_cast<int>(options)) = static_cast<int>(operation);
	}
}

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

void iAImNDTMain::generateOctrees(int maxLevel, int maxPointsPerRegion, vtkPolyData* dataSet)
{
	int lastLeafNodeAmount = 0;
	for(auto level = 0; level <= maxLevel; level++)
	{
		iAVROctree* tempOctree = new iAVROctree(m_vrEnv->renderer(), dataSet);
		tempOctree->calculateOctree(level, maxPointsPerRegion);
		// Check if leafNodes have been added, if not stop creating deeper levels
		if (tempOctree->getNumberOfLeafNodes() <= lastLeafNodeAmount)
		{
			break;
		}
		lastLeafNodeAmount = tempOctree->getNumberOfLeafNodes();

		m_octrees.push_back(tempOctree);
	}
	if(m_octrees.empty()) LOG(lvlDebug, QString("NO OCTREE GENERATED"));
}

void iAImNDTMain::calculateMetrics()
{
	m_MiMColorLegend->hide();
	m_MiMMip->hideMIPPanels();

	auto minMax = m_fiberMetrics->getMinMaxAvgRegionValues(m_currentOctreeLevel, m_currentFeature);
	m_MiMColorLegend->createLut(minMax.at(0), minMax.at(1), 1);
	std::vector<QColor> rgba = m_MiMColorLegend->getColors(m_currentOctreeLevel, m_currentFeature, m_fiberMetrics->getRegionAverage(m_currentOctreeLevel, m_currentFeature));
	m_MiMColorLegend->calculateLegend(m_vrEnv->getInitialWorldScale());

	if (m_modelInMiniatureActive)
	{
		m_modelInMiniature->applyHeatmapColoring(rgba); // Only call when model is calculated (poly data has to be accessible)

		m_MiMColorLegend->setTitle(QString(" %1 ").arg(m_fiberMetrics->getFeatureName(m_currentFeature)).toUtf8());

		m_MIPPanelsVisible = true;
		m_MiMMip->createSingleMIPPanel(m_currentOctreeLevel, m_currentFeature, m_viewDirection, m_vrEnv->getInitialWorldScale(), m_fiberMetrics->getRegionAverage(m_currentOctreeLevel, m_currentFeature));
	}
}

void iAImNDTMain::colorMiMCubes(std::vector<vtkIdType> const & regionIDs)
{
	auto rgba = m_MiMColorLegend->getColors(m_currentOctreeLevel, m_currentFeature, m_fiberMetrics->getRegionAverage(m_currentOctreeLevel, m_currentFeature));
	m_MiMColorLegend->calculateLegend(m_vrEnv->getInitialWorldScale());
	m_modelInMiniature->applyHeatmapColoring(rgba); //Reset Color
	
	for (size_t i = 0; i < regionIDs.size(); i++)
	{
		m_modelInMiniature->setCubeColor(OCTREE_COLOR, regionIDs.at(i));
	}
}

double iAImNDTMain::calculateWorldScaleFactor()
{
	double currentScale = m_vrEnv->interactor()->GetPhysicalScale();
	auto temp = m_vrEnv->getInitialWorldScale()/ currentScale;

	return temp;
}

void iAImNDTMain::changeOctreeAndMetric()
{
	iAVRTouchpadPosition touchpadPos = iAImNDTInteractions::getTouchedPadSide(m_touchPadPosition);
	if (touchpadPos == iAVRTouchpadPosition::Middle)
	{
		this->toggleArView();
		return;
	}

	if (touchpadPos == iAVRTouchpadPosition::Up || touchpadPos == iAVRTouchpadPosition::Down) {

		//m_octrees.at(currentOctreeLevel)->hide(); // Hide old octree
		m_volume->hide();
		m_volume->hideRegionLinks();
		m_volume->hideVolume();
		m_volume->resetVolume();
		addPropToOptionID(vtkProp3D::SafeDownCast(m_volume->getVolumeActor()), iAVRInteractionOptions::Volume);

		if (touchpadPos == iAVRTouchpadPosition::Up && m_currentOctreeLevel < static_cast<int>(m_octrees.size()) - 1)
		{
			m_currentOctreeLevel++;
		}
		if (touchpadPos == iAVRTouchpadPosition::Down && m_currentOctreeLevel > OCTREE_MIN_LEVEL)
		{
			m_currentOctreeLevel--;
		}

		QString text = QString("Octree Level %1").arg(m_currentOctreeLevel);
		m_3DTextLabels.at(0)->create3DLabel(text);
		m_3DTextLabels.at(0)->show();
		m_3DTextLabels.at(1)->hide();

		m_volume->setOctree(m_octrees.at(m_currentOctreeLevel));
		m_volume->createCubeModel();
		m_volume->show();
		m_volume->showVolume();
		m_networkGraphMode = false;
	}
	else if (touchpadPos == iAVRTouchpadPosition::Right || touchpadPos == iAVRTouchpadPosition::Left)
	{
		if (touchpadPos == iAVRTouchpadPosition::Right)
		{
			if (m_currentFeature < m_fiberMetrics->getNumberOfFeatures() - 1)
			{
				m_currentFeature++;
			}
			//m_octrees.at(currentOctreeLevel)->show();
		}
		if (touchpadPos == iAVRTouchpadPosition::Left)
		{
			if (m_currentFeature > 0)
			{
				m_currentFeature--;
			}
			//m_octrees.at(currentOctreeLevel)->hide();
		}

		m_3DTextLabels.at(0)->hide();
	}
	updateModelInMiniatureData();
}

void iAImNDTMain::pickSingleFiber(double eventPosition[3])
{
	std::vector<size_t> selection = std::vector<size_t>();
	// Find the closest points to TestPoint
	vtkIdType iD = m_octrees.at(m_currentOctreeLevel)->getOctree()->FindClosestPoint(eventPosition);

	vtkIdType rowiD = m_fiberCoverageCalc->getObjectiD(iD);
	selection.push_back(rowiD);

	m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
	emit selectionChanged();
}

void iAImNDTMain::pickFibersinRegion(double eventPosition[3], double eventOrientation[4])
{
	vtkIdType cellID = m_volume->getClosestCellID(eventPosition, eventOrientation);
	if (cellID >= 0 && cellID <= m_octrees.at(m_currentOctreeLevel)->getNumberOfLeafNodes())
	{
		pickFibersinRegion(cellID);
	}
}

void iAImNDTMain::pickFibersinRegion(int leafRegion)
{
	std::vector<size_t> selection = std::vector<size_t>();
	for (auto const & fiber : *m_fiberCoverageCalc->getObjectCoverage()->at(m_currentOctreeLevel).at(leafRegion))
	{
		//LOG(lvlImportant,QString("Nr. [%1]").arg(fiber.first));
		selection.push_back(fiber.first);
	}
	std::sort(selection.begin(), selection.end());

	m_multiPickIDs.push_back(leafRegion);
	if (m_modelInMiniatureActive)
	{
		m_modelInMiniature->highlightGlyphs(m_multiPickIDs);
	}
	m_volume->highlightGlyphs(m_multiPickIDs);

	m_multiPickIDs.clear();
	m_volume->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
	emit selectionChanged();
}

void iAImNDTMain::pickMimRegion(double eventPosition[3], double eventOrientation[4])
{
	vtkIdType cellID = m_modelInMiniature->getClosestCellID(eventPosition, eventOrientation);

	if (cellID >= 0)
	{
		auto keyPos = std::find(m_multiPickIDs.begin(), m_multiPickIDs.end(), cellID);
		//If the currently selected Region is already selected, then remove it (UNDO)
		if (keyPos != m_multiPickIDs.end())
		{
			m_multiPickIDs.erase(keyPos);
		}
		else
		{
			m_multiPickIDs.push_back(cellID);
		}
		
		m_modelInMiniature->highlightGlyphs(m_multiPickIDs);
		m_volume->highlightGlyphs(m_multiPickIDs);

		// If multitouch Key is not pressed render the single region it in the Volume
		if (m_activeInput.at(static_cast<int>(vtkEventDataDevice::RightController)) != static_cast<int>(iAVROperations::MultiPickMiMRegion))
		{
			pickFibersinRegion(m_multiPickIDs.at(0));
			m_multiPickIDs.clear();
		}
	}
}

void iAImNDTMain::multiPickMiMRegion()
{	
	if(!m_multiPickIDs.empty())
	{
		std::vector<size_t> selection = std::vector<size_t>();

		for (size_t i = 0; i < m_multiPickIDs.size(); i++)
		{
			for (auto const & fiber : *m_fiberCoverageCalc->getObjectCoverage()->at(m_currentOctreeLevel).at(m_multiPickIDs.at(i)))
			{
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
			m_multiPickIDs.resize(2); //only two regions allowed

			std::vector<int> featureList;
			//featureList->push_back(currentFeature); //
			featureList.push_back(21);
			featureList.push_back(20);
			featureList.push_back(19);
			featureList.push_back(11);
			featureList.push_back(18);
			featureList.push_back(17);
			featureList.push_back(7);

			auto cubePos = m_volume->getCubePos(m_multiPickIDs.at(0));
			auto cubeSize = m_volume->getCubeSize(m_multiPickIDs.at(0));
			auto visSize = m_vrEnv->interactor()->GetPhysicalScale() * 0.33; //33%
			
			m_distributionVis->createVisualization(cubePos, visSize, ceil(cubeSize), m_currentOctreeLevel, m_multiPickIDs, featureList); //
			m_distributionVis->show(); 
			m_distributionVis->rotateVisualization(180); //Initial Rotation
			m_volume->removeHighlightedGlyphs();
			m_volume->hideVolume();
			m_modelInMiniature->removeHighlightedGlyphs();

			auto coloring = m_distributionVis->getBarColors();
			m_volume->setNodeColor(m_multiPickIDs, coloring);
			m_modelInMiniature->highlightGlyphs(m_multiPickIDs, coloring);
			
			addPropToOptionID(vtkProp3D::SafeDownCast(m_distributionVis->getVisAssembly()), iAVRInteractionOptions::Histogram);//
		}
		m_multiPickIDs.clear();
	}
}

void iAImNDTMain::resetSelection()
{
	m_volume->renderSelection(std::vector<size_t>(), 0, QColor(140, 140, 140, 255), nullptr);
	emit selectionChanged();
	m_volume->removeHighlightedGlyphs();
	if (m_modelInMiniatureActive)
	{
		std::vector<QColor> rgba = m_MiMColorLegend->getColors(m_currentOctreeLevel, m_currentFeature, m_fiberMetrics->getRegionAverage(m_currentOctreeLevel, m_currentFeature));
		m_MiMColorLegend->calculateLegend(m_vrEnv->getInitialWorldScale());
		onZoom();
		m_modelInMiniature->applyHeatmapColoring(rgba); //Reset Color
		m_volume->resetNodeColor();
		m_modelInMiniature->removeHighlightedGlyphs();
		//fiberMetrics->hideMIPPanels();
	}
	m_distributionVis->hide();
}

void iAImNDTMain::updateModelInMiniatureData()
{
	int controllerID = static_cast<int>(vtkEventDataDevice::LeftController);
	
	m_modelInMiniature->setOctree(m_octrees.at(m_currentOctreeLevel));
	m_modelInMiniature->createCubeModel(); //Here a new MiM is calculated
	
	//m_networkGraphMode = false;
	resetSelection();

	m_modelInMiniature->getActor()->SetPosition(m_cPos[controllerID][0], m_cPos[controllerID][1], m_cPos[controllerID][2]);

	calculateMetrics();
	onZoom();
}

void iAImNDTMain::spawnModelInMiniature(double eventPosition[3], bool hide)
{
	if(!hide)
	{
		m_modelInMiniatureActive = true;
		updateModelInMiniatureData();
		onZoom(); //reset to current zoom
		m_modelInMiniature->show();
	
		m_MiMColorLegend->setPosition(eventPosition);
		m_MiMColorLegend->show();
	}
	else
	{
		m_modelInMiniatureActive = false;
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
	iAVRTouchpadPosition touchpadPos = iAImNDTInteractions::getTouchedPadSide(m_touchPadPosition);

	if (touchpadPos == iAVRTouchpadPosition::Middle)
	{
		this->toggleArView();
		return;
	}

	if (m_modelInMiniatureActive && m_currentOctreeLevel > 0)
	{
		m_MIPPanelsVisible = false;
		m_MiMMip->hideMIPPanels();

		if (touchpadPos == iAVRTouchpadPosition::Up || touchpadPos == iAVRTouchpadPosition::Down)
		{
			m_3DTextLabels.at(1)->show();

			if (touchpadPos == iAVRTouchpadPosition::Up)
			{
				//Just use positive offset
			}
			if (touchpadPos == iAVRTouchpadPosition::Down)
			{
				offsetMiM = offsetMiM * -1;
				offsetVol = offsetVol * -1;
			}

			switch (m_currentMiMDisplacementType)
			{
			case -1:
				LOG(lvlDebug, QString("Unknown Displacement!"));
				break;
			case 0:
				m_modelInMiniature->applySPDisplacement(offsetMiM);
				m_volume->applySPDisplacement(offsetVol);
				m_volume->moveFibersByMaxCoverage(m_fiberMetrics->getMaxCoverageFiberPerRegion(), offsetVol, true);
				break;
			case 1:
				m_modelInMiniature->applyRadialDisplacement(offsetMiM);
				m_volume->applyRadialDisplacement(offsetVol);
				m_volume->moveFibersByMaxCoverage(m_fiberMetrics->getMaxCoverageFiberPerRegion(), offsetVol, false);
				break;
			case 2:
				m_modelInMiniature->applyOctantDisplacement(offsetMiM);
				m_volume->applyOctantDisplacement(offsetVol);
				m_volume->moveFibersbyOctant(m_fiberMetrics->getMaxCoverageFiberPerRegion(), offsetVol);
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

		if (m_networkGraphMode)	m_volume->createSimilarityNetwork(m_fiberMetrics->getJaccardIndex(m_currentOctreeLevel), m_fiberMetrics->getMaxNumberOfFibersInRegion(m_currentOctreeLevel), initWorldScale);
	}
}

void iAImNDTMain::changeMiMDisplacementType()
{
	m_currentMiMDisplacementType++;

	if(m_currentMiMDisplacementType > 2)
	{
		m_currentMiMDisplacementType = 0;
	}

	switch (m_currentMiMDisplacementType)
	{
	case 0:
		m_3DTextLabels.at(1)->create3DLabel(QString("SP displacement"));
		break;
	case 1:
		m_3DTextLabels.at(1)->create3DLabel(QString("Radial displacement"));
		break;
	case 2:
		m_3DTextLabels.at(1)->create3DLabel(QString("Octant displacement"));
		break;
	}

	m_3DTextLabels.at(1)->show();
	m_3DTextLabels.at(0)->hide();	
}

void iAImNDTMain::flipDistributionVis()
{
	m_distributionVis->flipThroughHistograms(m_sign);
}

void iAImNDTMain::displayNodeLinkD()
{
	if (!m_networkGraphMode)
	{
		m_volume->hide();
		m_volume->hideVolume();
		m_volume->createSimilarityNetwork(m_fiberMetrics->getJaccardIndex(m_currentOctreeLevel), m_fiberMetrics->getMaxNumberOfFibersInRegion(m_currentOctreeLevel), m_vrEnv->getInitialWorldScale());
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
	if (m_arViewer->setEnabled(!m_arEnabled))
	{
		m_arEnabled = !m_arEnabled;
	}
}
