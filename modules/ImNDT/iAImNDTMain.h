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
#pragma once

#include "iACsvIO.h"
#include "iAVREnvironment.h"
#include "iAVRHistogramMetric.h"
#include "iAVRObjectCoverage.h"
#include "iAVROctreeMetrics.h"
#include "vtkEventData.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include <unordered_map>

class vtkOpenVRTrackedCamera;
class iAImNDTInteractorStyle;
class iAVR3DText;
class iAVRColorLegend;
class iAVRColorLegend;
class iAVRFrontCamera;
class iAVRHistogramPairVis;
class iAVRMip;
class iAVRModelInMiniature;
class iAVRObjectModel;
class iAVROctree;
class iAVRSlider;

// Enumeration of different interaction options for different Objects
enum class iAVRInteractionOptions {
  Unknown = -1,
  NoInteractions,
  NoObject,
  Anywhere,
  MiniatureModel,
  Volume,
  Histogram,
  NumberOfInteractionOptions
};

// Enumeration of different Operations
enum class iAVROperations {
  Unknown = -1,
  None,
  SpawnModelInMiniature,
  PickFibersinRegion,
  PickMiMRegion,
  MultiPickMiMRegion,
  ChangeOctreeAndMetric,
  ResetSelection,
  ExplodeMiM,
  DisplayNodeLinkDiagram,
  ChangeMiMDisplacementType,
  ChangeJaccardIndex,
  FlipHistoBookPages,
  NumberOfOperations
};

//!
class iAImNDTMain
{
public:
	iAImNDTMain(iAVREnvironment* vrEnv, iAImNDTInteractorStyle* style, vtkTable* objectTable, iACsvIO io, iACsvConfig csvConfig, std::map<size_t, std::vector<iAVec3f> > curvedFiberInfo);
	void startInteraction(vtkEventDataDevice3D* device, vtkProp3D* pickedProp, double eventPosition[3], double eventOrientation[4]); //Press, Touch
	void endInteraction(vtkEventDataDevice3D* device, vtkProp3D* pickedProp, double eventPosition[3], double eventOrientation[4]); //Release, Untouch
	void onMove(vtkEventDataDevice3D* device, double movePosition[3], double eventOrientation[4]); //Movement
	void onZoom();
	bool toggleArView();
	vtkIdType currentOctreeLevel;

private:
	iAVREnvironment* m_vrEnv;
	std::vector<iAVROctree*>* m_octrees;
	iAVRModelInMiniature* m_modelInMiniature;
	iAVRObjectModel* m_volume;
	vtkSmartPointer<vtkPolyData> m_extendedCylinderVisData; // Data extended with additional intersection points
	vtkSmartPointer<iAImNDTInteractorStyle> m_style;
	vtkSmartPointer<vtkTable> m_objectTable;

	bool m_networkGraphMode;
	std::vector<iAVR3DText*>* m_3DTextLabels;
	iAVRObjectCoverage* m_fiberCoverageCalc;
	iAVRSlider* m_slider;
	iAVRColorLegend* m_MiMColorLegend;
	iAVRMip* m_MiMMip;
	iACsvIO m_io;
	std::map<size_t, std::vector<iAVec3f> > m_curvedFiberInfo;
	iAVROctreeMetrics* fiberMetrics;
	iAVRHistogramMetric* histogramMetrics;
	iAVRHistogramPairVis* m_distributionVis;
	int currentFeature;
	int currentMiMDisplacementType;
	std::vector<vtkIdType>* multiPickIDs;
	//Current Device Position
	double cPos[vtkEventDataNumberOfDevices][3];
	//Current Device Orientation
	double cOrie[vtkEventDataNumberOfDevices][4];
	//Current Focal Point
	double focalPoint[3];
	//Current view direction of the head
	int viewDirection;
	//Current touchpad Position
	float touchPadPosition[3];
	// Active Input Saves the current applied Input in case Multiinput is requires
	std::vector<int>* activeInput;
	// Map Actors to iAVRInteractionOptions
	std::unordered_map<vtkProp3D*, int> m_ActorToOptionID;
	// True if the corresponding actor is visible
	bool modelInMiniatureActive = false;
	// True if the MIP Panels should be visible
	bool m_MIPPanelsVisible = false;
	double m_rotationOfDisVis;
	double controllerTravelledDistance;
	int sign;
	vtkSmartPointer<vtkActor> pointsActor;

	iAVRFrontCamera* arViewer;
	bool arEnabled = false;

	void setInputScheme(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, iAVRInteractionOptions options, iAVROperations operation);
	int getOptionForObject(vtkProp3D* pickedProp);
	void addPropToOptionID(vtkProp3D* prop, iAVRInteractionOptions iD);
	void drawPoint(std::vector<double*>* pos, QColor color);
	void generateOctrees(int maxLevel, int maxPointsPerRegion, vtkPolyData* dataSet);
	void calculateMetrics();
	void updateModelInMiniatureData();
	void colorMiMCubes(std::vector<vtkIdType>* regionIDs);
	double calculateWorldScaleFactor();

	//# Methods for interaction #//
	void changeOctreeAndMetric();
	void pickSingleFiber(double eventPosition[3]);
	void pickFibersinRegion(double eventPosition[3], double eventOrientation[4]);
	void pickFibersinRegion(int leafRegion);
	void pickMimRegion(double eventPosition[3], double eventOrientation[4]);
	void multiPickMiMRegion();
	void resetSelection();
	void spawnModelInMiniature(double eventPosition[3], bool hide);
	void pressLeftTouchpad();
	void changeMiMDisplacementType();
	void flipDistributionVis();
	void displayNodeLinkD();
	
};
