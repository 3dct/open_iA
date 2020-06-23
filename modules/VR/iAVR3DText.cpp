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
#include "iAVR3DText.h"

#include <iAConsole.h>
#include "vtkTextProperty.h"

iAVR3DText::iAVR3DText(vtkRenderer* ren): m_renderer(ren)
{
	m_textActor3D = vtkSmartPointer<vtkBillboardTextActor3D>::New();
	m_visible = false;

	for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
	{
		for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
		{
			this->ControlsHelpers[d][i] = nullptr;
		}
	}
}

void iAVR3DText::show()
{
	if (m_visible)
	{
		return;
	}
	m_renderer->AddActor(m_textActor3D);
	m_visible = true;
}

void iAVR3DText::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_renderer->RemoveActor(m_textActor3D);
	m_visible = false;
}

void iAVR3DText::draw3DLable(double pos[3], QString text)
{
	m_textActor3D->SetPosition(pos);
	m_textActor3D->SetScale(1,1,1);
	m_textActor3D->SetInput(text.toUtf8());

	m_textActor3D->GetTextProperty()->SetFrame(1);
	m_textActor3D->GetTextProperty()->SetFrameColor(1.0, 1.0, 1.0);
	m_textActor3D->GetTextProperty()->SetBackgroundOpacity(1.0);
	m_textActor3D->GetTextProperty()->SetBackgroundColor(0.0, 0.0, 0.0);
	m_textActor3D->GetTextProperty()->SetFontSize(30);
}

void iAVR3DText::updatePos(double pos[3])
{
	m_textActor3D->SetPosition(pos);
}

void iAVR3DText::drawInputTooltip(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, QString text)
{
	int deviceID = static_cast<int>(device); // Device
	int inputID = static_cast<int>(input);  // Input Method
	int actioniD = static_cast<int>(action);     // Action of Input Method

	vtkStdString controlName = vtkStdString();
	vtkStdString controlText = vtkStdString();
	int drawSide = -1;
	int buttonSide = -1;

	// Clean already existing helpers
	if (ControlsHelpers[deviceID][inputID] != nullptr)
	{
		if (m_renderer)
		{
			m_renderer->RemoveViewProp(ControlsHelpers[deviceID][inputID]);
		}
		ControlsHelpers[deviceID][inputID]->Delete();
		ControlsHelpers[deviceID][inputID] = nullptr;
	}

	// Setup default text and layout
	switch (input)
	{
	case vtkEventDataDeviceInput::Trigger:
		controlName = "trigger";
		drawSide = vtkOpenVRControlsHelper::Left;
		buttonSide = vtkOpenVRControlsHelper::Back;
		controlText = "Trigger :\n";
		break;
	case vtkEventDataDeviceInput::TrackPad:
		controlName = "trackpad";
		drawSide = vtkOpenVRControlsHelper::Right;
		buttonSide = vtkOpenVRControlsHelper::Front;
		controlText = "Trackpad :\n";
		break;
	case vtkEventDataDeviceInput::Grip:
		controlName = "lgrip";
		drawSide = vtkOpenVRControlsHelper::Right;
		buttonSide = vtkOpenVRControlsHelper::Back;
		controlText = "Grip :\n";
		break;
	case vtkEventDataDeviceInput::ApplicationMenu:
		controlName = "button";
		drawSide = vtkOpenVRControlsHelper::Left;
		buttonSide = vtkOpenVRControlsHelper::Front;
		controlText = "Application Menu :\n";
		break;
	}

	if (text.toUtf8() != "")
	{
		controlText += text.toUtf8();
	
		// Create an input helper and add it to the renderer
		vtkOpenVRControlsHelper* inputHelper = vtkOpenVRControlsHelper::New();
		inputHelper->SetTooltipInfo(text.toUtf8(), buttonSide, drawSide, controlText.c_str());

		ControlsHelpers[deviceID][inputID] = inputHelper;
		ControlsHelpers[deviceID][inputID]->SetDevice(device);
	}
}

void iAVR3DText::showInputTooltip()
{
	for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
	{
		for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
		{
			if(ControlsHelpers[d][i] != nullptr)
			{
				ControlsHelpers[d][i]->SetRenderer(m_renderer);
				ControlsHelpers[d][i]->BuildRepresentation();
				m_renderer->AddViewProp(ControlsHelpers[d][i]);
				ControlsHelpers[d][i]->SetEnabled(true);
			}
		}
	}
	
}

void iAVR3DText::updateInputTooltip()
{
	for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
	{
		for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
		{
			if (ControlsHelpers[d][i] != nullptr)
			{
				ControlsHelpers[d][i]->UpdateRepresentation();
			}
		}
	}
}
