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

#include <vtkOpenVRInteractorStyle.h>
#include <vtkSmartPointer.h>
#include "vtkEventData.h"
#include "iAVRMain.h"

#define NUMBER_OF_DEVICES static_cast<int>(vtkEventDataDevice::NumberOfDevices)
#define NUMBER_OF_INPUTS static_cast<int>(vtkEventDataDeviceInput::NumberOfInputs)
#define NUMBER_OF_ACTIONS static_cast<int>(vtkEventDataAction::NumberOfActions)
#define NUMBER_OF_OPTIONS static_cast<int>(iAVRInteractionOptions::NumberOfInteractionOptions)

using inputScheme = std::vector < std::vector < std::vector<std::vector<int>>>>;

//! Base Class for specific interaction callbacks
class iAVRInteractorStyle : public vtkOpenVRInteractorStyle
{
   public:
	static iAVRInteractorStyle* New();
	vtkTypeMacro(iAVRInteractorStyle, vtkOpenVRInteractorStyle);

	void setVRMain(iAVRMain* vrMain);
	void OnButton3D(vtkEventData* edata) override;
	inputScheme* getInputScheme();	// returns the vector for the Operation definition
	std::vector<int>* getActiveInput(); //if >0 then has an action applied

   protected:
	iAVRInteractorStyle();

   private:
	iAVRMain* m_vrMain;
	double m_eventPosition[3];
	inputScheme* m_inputScheme;
	std::vector<int>* m_activeInput;

};
