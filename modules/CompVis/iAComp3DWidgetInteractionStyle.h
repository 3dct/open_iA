// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkInteractorStyleTrackballCamera.h>

class iAComp3DWidget;

class vtkRenderer;

class iAComp3DWidgetInteractionStyle : public vtkInteractorStyleTrackballCamera
{
public:

	static iAComp3DWidgetInteractionStyle* New();
	vtkTypeMacro(iAComp3DWidgetInteractionStyle, vtkInteractorStyleTrackballCamera);

	/*** Initialization ***/
	void setVisualization(iAComp3DWidget* visualization);
	

	/*** Interaction ***/
	virtual void OnLeftButtonDown() override;



protected:
	iAComp3DWidgetInteractionStyle();
	
private:

	iAComp3DWidget* m_visualization;

};
