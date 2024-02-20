// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVR3DText.h"

#include <iAVec3.h>

#include <vtkBillboardTextActor3D.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>

#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
#include <vtkVRControlsHelper.h>
#else
#include <vtkOpenVRControlsHelper.h>
#endif

iAVR3DText::iAVR3DText(vtkRenderer* ren): m_renderer(ren)
{
	m_textActor3D = vtkSmartPointer<vtkBillboardTextActor3D>::New();
	m_visible = false;
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

void iAVR3DText::create3DLabel(QString const & text)
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

void iAVR3DText::createSmall3DLabel(QString const& text)
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

void iAVR3DText::transformPosition(vtkTransform* transform)
{
	m_textActor3D->SetPosition(transform->TransformPoint(m_textActor3D->GetPosition()));
	m_textActor3D->Modified();
}
