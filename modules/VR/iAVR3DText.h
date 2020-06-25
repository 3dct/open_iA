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

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include "vtkBillboardTextActor3D.h"
#include "vtkOpenVRControlsHelper.h"

#include <QString>

//! Creates 3D Labels in the VR Environment
class iAVR3DText
{
public:
	iAVR3DText(vtkRenderer* ren);
	void create3DLabel(QString text);
	void setLabelPos(double pos[3]);
	void drawInputTooltip(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, QString text);
	void moveInEyeDir(double x, double y, double z);
	void showInputTooltip();
	void updateInputTooltip();
	void show();
	void hide();

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkBillboardTextActor3D> m_textActor3D;
	vtkOpenVRControlsHelper* ControlsHelpers[vtkEventDataNumberOfDevices][vtkEventDataNumberOfInputs];
	bool m_visible;
};