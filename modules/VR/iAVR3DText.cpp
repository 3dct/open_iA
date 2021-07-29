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

#include <iALog.h>
#include <iAVec3.h>

#include "vtkTextProperty.h"
#include "vtkCamera.h"

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

void iAVR3DText::create3DLabel(QString text)
{
	m_textActor3D->SetScale(1,1,1);
	m_textActor3D->SetInput(text.toUtf8());
	m_textActor3D->PickableOff();

	m_textActor3D->GetTextProperty()->SetJustificationToCentered();
	m_textActor3D->GetTextProperty()->SetFrame(1);
	m_textActor3D->GetTextProperty()->SetFrameColor(0.6, 0.6, 0.6);
	m_textActor3D->GetTextProperty()->SetFrameWidth(4);
	m_textActor3D->GetTextProperty()->SetBackgroundOpacity(1.0);
	m_textActor3D->GetTextProperty()->SetBackgroundColor(0.4, 0.4, 0.4);
	m_textActor3D->GetTextProperty()->SetFontSize(32);
}

void iAVR3DText::createSmall3DLabel(QString text)
{
	m_textActor3D->SetScale(1, 1, 1);
	m_textActor3D->SetInput(text.toUtf8());
	m_textActor3D->PickableOff();

	m_textActor3D->GetTextProperty()->SetJustificationToCentered();
	m_textActor3D->GetTextProperty()->SetFrame(1);
	m_textActor3D->GetTextProperty()->SetFrameColor(0.6, 0.6, 0.6);
	m_textActor3D->GetTextProperty()->SetFrameWidth(2);
	m_textActor3D->GetTextProperty()->SetBackgroundOpacity(1.0);
	m_textActor3D->GetTextProperty()->SetBackgroundColor(0.4, 0.4, 0.4);
	m_textActor3D->GetTextProperty()->SetFontSize(14);
	
}

void iAVR3DText::setLabelPos(double pos[3])
{
	m_textActor3D->SetPosition(pos);
}

void iAVR3DText::moveInEyeDir(double x, double y, double z)
{
	iAVec3d eye = iAVec3d(m_renderer->GetActiveCamera()->GetPosition());
	iAVec3d currentPos = iAVec3d(m_textActor3D->GetPosition());
	iAVec3d normDir = eye - currentPos;
	normDir.normalize();

	m_textActor3D->AddPosition(normDir[0] *x, normDir[1] * y, normDir[2] * z);
}

vtkSmartPointer<vtkBillboardTextActor3D> iAVR3DText::getTextActor()
{
	return m_textActor3D;
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

