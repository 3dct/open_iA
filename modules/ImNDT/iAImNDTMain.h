// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImNDTInteractorStyle.h"
#include "iAVREnvironment.h"
#include "iAVRHistogramMetric.h"
#include "iAVRObjectCoverage.h"
#include "iAVROctreeMetrics.h"

#include <vtkEventData.h>
#include <vtkSmartPointer.h>

#include <unordered_map>

class iAObjectsData;

class iAVR3DText;
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
  ChangeOctreeAndMetric,     //  5
  ResetSelection,
  ExplodeMiM,
  DisplayNodeLinkDiagram,
  ChangeMiMDisplacementType,
  ChangeJaccardIndex,        // 10
  FlipHistoBookPages,
  LeftGrid,
  NumberOfOperations
};

//!
class iAImNDTMain: public QObject
{
	Q_OBJECT
public:
	iAImNDTMain(iAVREnvironment* vrEnv, iAColoredPolyObjectVis* polyObject, iAObjectsData const * data, iACsvConfig csvConfig);
	// required to be able to forward-declare for unique_ptr on clang:
	~iAImNDTMain();
	//! Defines the action executed for specific controller inputs
	//! Position and Orientation are in WorldCoordinates and Orientation is in Degree
	void startInteraction(vtkEventDataDevice3D* device, vtkProp3D* pickedProp, double eventPosition[3], double eventOrientation[4]); //Press, Touch
	void endInteraction(vtkEventDataDevice3D* device, vtkProp3D* pickedProp, double eventPosition[3], double eventOrientation[4]); //Release, Untouch
	void onMove(vtkEventDataDevice3D* device, double movePosition[3], double eventOrientation[4]); //Movement
	//! Corrects the size of elements based on the new physical scale of the environment
	void onZoom();
	vtkIdType currentOctreeLevel;

signals:
	void selectionChanged();

private:
	iAVREnvironment* m_vrEnv;
	std::vector<iAVROctree*>* m_octrees;
	iAVRModelInMiniature* m_modelInMiniature;
	iAVRObjectModel* m_volume;
	vtkSmartPointer<vtkPolyData> m_extendedCylinderVisData; // Data extended with additional intersection points
	iAImNDTInteractions m_interactions;
	iAColoredPolyObjectVis* m_polyObject;

	bool m_networkGraphMode;
	std::vector<iAVR3DText*>* m_3DTextLabels;
	iAVRObjectCoverage* m_fiberCoverageCalc;
	iAVRSlider* m_slider;
	iAVRColorLegend* m_MiMColorLegend;
	iAVRMip* m_MiMMip;
	iAVROctreeMetrics* fiberMetrics;
	iAVRHistogramMetric* histogramMetrics;
	iAVRHistogramPairVis* m_distributionVis;
	int currentFeature;
	int currentMiMDisplacementType;
	std::vector<vtkIdType>* multiPickIDs;
	//! Current Device Position
	double cPos[vtkEventDataNumberOfDevices][3];
	//! Current Device Orientation
	double cOrie[vtkEventDataNumberOfDevices][4];
	//! Current Focal Point
	double focalPoint[3];
	//! Current view direction of the head
	int viewDirection;
	//! Current touchpad Position
	float m_touchPadPosition[3];
	//! Active Input Saves the current applied Input in case Multiinput is requires
	std::vector<int>* activeInput;
	//! Map Actors to iAVRInteractionOptions
	std::unordered_map<vtkProp3D*, int> m_ActorToOptionID;
	//! True if the corresponding actor is visible
	bool modelInMiniatureActive = false;
	//! True if the MIP Panels should be visible
	bool m_MIPPanelsVisible = false;
	double m_rotationOfDisVis;
	double controllerTravelledDistance;
	int sign;
	vtkSmartPointer<vtkActor> pointsActor;

	std::unique_ptr<iAVRFrontCamera> m_arViewer;
	bool m_arEnabled = false;

	void setInputScheme(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, iAVRInteractionOptions options, iAVROperations operation);
	//! Returns which InteractionOption is for the currently picked Object available 
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
	void toggleArView();
};
