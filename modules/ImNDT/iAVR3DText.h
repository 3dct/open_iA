// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	void createSmall3DLabel(QString text);
	void setLabelPos(double pos[3]);
	void moveInEyeDir(double x, double y, double z);
	vtkSmartPointer<vtkBillboardTextActor3D> getTextActor();
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
