// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <iAGUIModuleInterface.h>

// Labelling
#include <iAAnnotationTool.h>    // for iAAnnotation (maybe extract to separate .h file?)

#include <vtkSmartPointer.h>

class vtkCaptionWidget;

//! Currently handles multiple annotation tools in different children
//! by just making all of them available in VR
class iAVRAnnotationsModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	iAVRAnnotationsModuleInterface();
	void Initialize() override;

private:
	void recreateVRAnnotations();
	//! list of all annotations across all children with active annotations tool:
	std::map<iAMdiChild*, std::vector<iAAnnotation>> m_annotations;
	std::vector<vtkSmartPointer<vtkCaptionWidget>> m_captionWidgets;
};
