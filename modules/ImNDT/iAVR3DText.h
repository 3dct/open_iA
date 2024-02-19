// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkEventData.h>    // for vtkEventDataNumberOfDevices
#include <vtkSmartPointer.h>
#include <vtkVersionMacros.h>

#include <QString>

class vtkBillboardTextActor3D;
class vtkRenderer;
class vtkTransform;

#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
class vtkVRControlsHelper;
using iAvtkVRControlsHelper = vtkVRControlsHelper;
#else
class vtkOpenVRControlsHelper;
using iAvtkVRControlsHelper = vtkOpenVRControlsHelper;
#endif

//! Creates 3D Labels in the VR Environment
class iAVR3DText
{
public:
	iAVR3DText(vtkRenderer* ren);
	void create3DLabel(QString const& text);
	void createSmall3DLabel(QString const& text);
	void setLabelPos(double pos[3]);
	void moveInEyeDir(double x, double y, double z);
	void transformPosition(vtkTransform* transform);
	void show();
	void hide();

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkBillboardTextActor3D> m_textActor3D;
	bool m_visible;
};
