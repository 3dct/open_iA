// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImNDTInteractorStyle.h"    // for iAImNDTInteractions

#include <iACsvConfig.h>

#include <vtkEventData.h>
#include <vtkSmartPointer.h>

#include <QObject>

#include <unordered_map>

class iAColoredPolyObjectVis;
class iAObjectsData;

class iAVR3DText;
class iAVRColorLegend;
class iAVREnvironment;
class iAVRFrontCamera;
class iAVRHistogramMetric;
class iAVRHistogramPairVis;
class iAVRMip;
class iAVRModelInMiniature;
class iAVRObjectCoverage;
class iAVRObjectModel;
class iAVROctree;
class iAVROctreeMetrics;
class vtkPolyData;
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
	//! Destructor, required to be able to forward-declare for unique_ptr on clang:
	~iAImNDTMain();
	//! Defines the action executed for specific controller inputs
	//! Position and Orientation are in WorldCoordinates and Orientation is in Degree
	void startInteraction(vtkEventDataDevice3D* device, vtkProp3D* pickedProp, double eventPosition[3], double eventOrientation[4]); //Press, Touch
	void endInteraction(vtkEventDataDevice3D* device, vtkProp3D* pickedProp, double eventPosition[3], double eventOrientation[4]); //Release, Untouch
	void onMove(vtkEventDataDevice3D* device, double movePosition[3], double eventOrientation[4]); //Movement
	//! Corrects the size of elements based on the new physical scale of the environment
	void onZoom();
	//! shut down object rendering
	void stop();

signals:
	void selectionChanged();
	void finished();

private:
	using InputScheme = std::vector<std::vector<std::vector<std::vector<int>>>>;

	vtkIdType m_currentOctreeLevel;

	iAVREnvironment* m_vrEnv;
	std::vector<iAVROctree*> m_octrees;
	iAVRModelInMiniature* m_modelInMiniature;
	iAVRObjectModel* m_volume;
	vtkSmartPointer<vtkPolyData> m_extendedCylinderVisData; // Data extended with additional intersection points
	iAImNDTInteractions m_interactions;
	iAColoredPolyObjectVis* m_polyObject;
	InputScheme m_inputScheme;
	std::vector<int> m_activeInput;

	bool m_networkGraphMode;
	std::vector<iAVR3DText*> m_3DTextLabels;
	iAVRObjectCoverage* m_fiberCoverageCalc;
	iAVRSlider* m_slider;
	iAVRColorLegend* m_MiMColorLegend;
	iAVRMip* m_MiMMip;
	iAVROctreeMetrics* m_fiberMetrics;
	iAVRHistogramMetric* m_histogramMetrics;
	iAVRHistogramPairVis* m_distributionVis;
	int m_currentFeature;
	int m_currentMiMDisplacementType;
	std::vector<vtkIdType> m_multiPickIDs;
	//! Current Device Position
	double m_cPos[vtkEventDataNumberOfDevices][3];
	//! Current Device Orientation
	double m_cOrie[vtkEventDataNumberOfDevices][4];
	//! Current Focal Point
	double m_focalPoint[3];
	//! Current view direction of the head
	int m_viewDirection;
	//! Current touchpad Position
	float m_touchPadPosition[3];
	//! Map Actors to iAVRInteractionOptions
	std::unordered_map<vtkProp3D*, int> m_ActorToOptionID;
	//! True if the corresponding actor is visible
	bool m_modelInMiniatureActive = false;
	//! True if the MIP Panels should be visible
	bool m_MIPPanelsVisible = false;
	double m_rotationOfDisVis;
	double m_controllerTravelledDistance;
	int m_sign;
	vtkSmartPointer<vtkActor> m_pointsActor;

	std::unique_ptr<iAVRFrontCamera> m_arViewer;
	bool m_arEnabled = false;

	void setInputScheme(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, iAVRInteractionOptions options, iAVROperations operation);
	//! Returns which InteractionOption is for the currently picked Object available 
	int getOptionForObject(vtkProp3D* pickedProp);
	void addPropToOptionID(vtkProp3D* prop, iAVRInteractionOptions iD);
	//! Generates octrees until maxLevel is reached or no more leafNodes are added through later levels
	void generateOctrees(int maxLevel, int maxPointsPerRegion, vtkPolyData* dataSet);
	void calculateMetrics();
	//! Updates the data (Octree, Metrics,...) and the position for the current MiM
	void updateModelInMiniatureData();
	void colorMiMCubes(std::vector<vtkIdType> const & regionIDs);
	//! Returns the difference between the intial world scaling and the current scaling
	double calculateWorldScaleFactor();

	//# Methods for interaction #//

	//! Increases/Decreases the current octree level and feature. Recalculates the Model in Miniature Object.
	void changeOctreeAndMetric();
	void pickSingleFiber(double eventPosition[3]);
	//! Picks all fibers in the region clicked by the user.
	void pickFibersinRegion(double eventPosition[3], double eventOrientation[4]);
	//! Picks all fibers in the octree region defined by the leaf node ID.
	void pickFibersinRegion(int leafRegion);
	void pickMimRegion(double eventPosition[3], double eventOrientation[4]);
	//! Methods ends the multi picking mode and renders selection
	void multiPickMiMRegion();
	void resetSelection();
	void spawnModelInMiniature(double eventPosition[3], bool hide);
	void pressLeftTouchpad();
	void changeMiMDisplacementType();
	void flipDistributionVis();
	void displayNodeLinkD();
	void toggleArView();
};
