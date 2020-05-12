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

#include "vtkEventData.h"
#include "vtkTable.h"
#include "vtkDataSet.h"
#include "vtkProp3D.h"
#include "iACsvIO.h"

#include "iAVRInteractorStyle.h"


// Enumeration of different interaction options for different Objects
enum class iAVRInteractionOptions {
  Unknown = -1,
  Default,
  MiniatureModel,
  Volume,
  NumberOfOptions
};

// Enumeration of different Operations
enum class iAVROperations {
  Unknown = -1,
  None,
  SpawnMM,
  ArrangeObject,
  WorldGrab,
  WorldZoom,
  Slicing,
  NumberOfOperations
};

class iAVR3DObjectVis;
class iA3DCylinderObjectVis;
class iAVROctree; 

//! Class for  
class iAVRMain
{
public:
	iAVRMain(iAVREnvironment* vrEnv, iAVRInteractorStyle* style, vtkTable* objectTable, iACsvIO io);
	void startInteraction(vtkEventDataDevice3D* device, double eventPosition[3], vtkProp3D* m_pickedProp); //Press, Touch
	void endInteraction(); //Release, Untouch
	int octreeLevel;
private:
	iAVREnvironment* m_vrEnv;
	iAVROctree* m_octree;
	iAVR3DObjectVis* m_objectVis;
	iA3DCylinderObjectVis* m_cylinderVis;
	vtkSmartPointer<iAVRInteractorStyle> m_style;
	vtkSmartPointer<vtkTable> m_objectTable;
	vtkSmartPointer<vtkProp3D> m_pickedProp;
	iACsvIO m_io;

	vtkIdType getObjectiD(double pos[3]);
	bool checkEqualArrays(float pos1[3], float pos2[3]);
};
