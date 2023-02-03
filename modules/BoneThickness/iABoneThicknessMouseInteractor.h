// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>
#include <vtkSetGet.h>

class iABoneThickness;
class iABoneThicknessChartBar;
class iABoneThicknessTable;

class vtkActorCollection;
class vtkPropPicker;

class iABoneThicknessMouseInteractor : public vtkInteractorStyleTrackballCamera
{
public:
	static iABoneThicknessMouseInteractor* New();
	vtkTypeMacro(iABoneThicknessMouseInteractor, vtkInteractorStyleTrackballCamera);
	void set ( iABoneThickness* _pBoneThickness
			 , iABoneThicknessChartBar* _pBoneThicknessChartBar, iABoneThicknessTable* _pBoneThicknessTable
			 , vtkActorCollection* _pSpheres );
	void OnLeftButtonDown() override;

private:
	iABoneThickness* m_pBoneThickness = nullptr;
	iABoneThicknessChartBar* m_pBoneThicknessChartBar = nullptr;
	iABoneThicknessTable* m_pBoneThicknessTable = nullptr;

	vtkSmartPointer<vtkPropPicker> m_pPicker;
	vtkRenderer* m_pRenderer = nullptr;
	vtkActorCollection* m_pSpheres = nullptr;
};
