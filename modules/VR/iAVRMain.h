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
#pragma once

#include "vtkSmartPointer.h"
#include "iAVREnvironment.h"
#include "iAVRMetrics.h"

#include "vtkEventData.h"
#include "vtkTable.h"
#include "vtkDataSet.h"
#include "vtkProp3D.h"
#include "vtkPolyData.h"
#include <vtkPlaneSource.h>
#include "iACsvIO.h"

#include <unordered_map>
#include <thread>

// Enumeration of different interaction options for different Objects
enum class iAVRInteractionOptions {
  Unknown = -1,
  NoObject,
  Anywhere,
  MiniatureModel,
  Volume,
  NumberOfInteractionOptions
};

// Enumeration of different Operations
enum class iAVROperations {
  Unknown = -1,
  None,
  SpawnModelInMiniature,
  PickSingleFiber,
  PickFibersinRegion,
  PickMiMRegion,
  ChangeOctreeLevel,
  ResetSelection,
  NumberOfOperations
};

class iAVR3DObjectVis;
class iA3DCylinderObjectVis;
class iAVROctree; 
class iAVRInteractorStyle;

//!
class iAVRMain
{
public:
	iAVRMain(iAVREnvironment* vrEnv, iAVRInteractorStyle* style, vtkTable* objectTable, iACsvIO io);
	void startInteraction(vtkEventDataDevice3D* device, double eventPosition[3], double eventOrientation[4], vtkProp3D* pickedProp); //Press, Touch
	void endInteraction(vtkEventDataDevice3D* device, double eventPosition[3], double eventOrientation[4],vtkProp3D* pickedProp); //Release, Untouch
	void onMove(vtkEventDataDevice3D* device, double movePosition[3], double eventOrientation[4], vtkProp3D* pickedProp); //Movement
	int currentOctreeLevel;

private:
	iAVREnvironment* m_vrEnv;
	std::vector<iAVROctree*>* m_octrees;
	iAVR3DObjectVis* m_objectVis;
	iA3DCylinderObjectVis* m_cylinderVis;
	vtkSmartPointer<vtkPolyData> m_extendedCylinderVisData; // Data extended with additional intersection points
	vtkSmartPointer<iAVRInteractorStyle> m_style;
	vtkSmartPointer<vtkTable> m_objectTable;
	iAVRMetrics* fiberMetrics;
	iACsvIO m_io;
	//Current Device Position
	double cPos[vtkEventDataNumberOfDevices][3];
	// Map Actors to iAVRInteractionOptions
	std::unordered_map<vtkProp3D*, int> m_ActorToOptionID;
	// Maps poly point IDs to Object IDs in csv file
	std::unordered_map<vtkIdType, vtkIdType> m_pointIDToCsvIndex;
	std::thread m_iDMappingThread;
	// True if the Thread has not joined yet
	bool m_iDMappingThreadRunning = true;
	// True if the corresponding actor is visible
	bool modelInMiniatureActive = false;
	//Stores for the [octree level] in an [octree region] a map of its fiberIDs with their coverage
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* m_fiberCoverage;
	vtkPoints* newIntersectionPoints;

	void mapAllPointiDs();
	void mapAllPointiDsAndCalculateFiberCoverage();
	vtkIdType getObjectiD(vtkIdType polyPoint);
	vtkIdType mapSinglePointiD(vtkIdType polyPoint);
	bool checkEqualArrays(float pos1[3], float pos2[3]);
	void setInputScheme(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, iAVRInteractionOptions options, iAVROperations operation);
	int getOptionForObject(vtkProp3D* pickedProp);
	void addPropToOptionID(vtkProp3D* prop, iAVRInteractionOptions iD);
	bool checkIntersectionWithBox(double startPoint[3], double endPoint[3], std::vector<vtkSmartPointer<vtkPlaneSource>>* planes, double bounds[6], double intersection[3]);
	double calculateFiberCoverage(double startPoint[3], double endPoint[3], double fiberLength);
	vtkSmartPointer<vtkPoints> getOctreeFiberCoverage(double startPoint[3], double endPoint[3], int octreeLevel, double fiberLength, std::vector<double>* coverageInRegion);
	void drawPoint(std::vector<double*>* pos, QColor color);
	void generateOctrees(int maxLevel, int maxPointsPerRegion, vtkPolyData* dataSet);
	void calculateMetrics();
	void updateModelInMiniatureData();

	//# Methods for interaction #//
	void changeOctreeLevel();
	void pickSingleFiber(double eventPosition[3]);
	void pickFibersinRegion(double eventPosition[3]);
	void pickFibersinRegion(int leafRegion);
	void pickMimRegion(double eventPosition[3], double eventOrientation[4]);
	void resetSelection();
	void spawnModelInMiniature(double eventPosition[3], bool hide);
};
