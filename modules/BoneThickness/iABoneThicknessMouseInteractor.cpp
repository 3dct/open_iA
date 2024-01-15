// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iABoneThicknessMouseInteractor.h"

#include "iABoneThickness.h"
#include "iABoneThicknessChartBar.h"
#include "iABoneThicknessTable.h"

#include <vtkActorCollection.h>
#include <vtkObjectFactory.h>
#include <vtkPropPicker.h>
#include <vtkRenderWindowInteractor.h>

vtkStandardNewMacro(iABoneThicknessMouseInteractor);

void iABoneThicknessMouseInteractor::set(iABoneThickness* _pBoneThickness
	, iABoneThicknessChartBar* _pBoneThicknessChartBar, iABoneThicknessTable* _pBoneThicknessTable
	, vtkActorCollection* _pSpheres
)
{
	m_pBoneThickness = _pBoneThickness;
	m_pBoneThicknessChartBar = _pBoneThicknessChartBar;
	m_pBoneThicknessTable = _pBoneThicknessTable;

	m_pPicker = vtkSmartPointer<vtkPropPicker>::New();
	m_pRenderer = GetDefaultRenderer();
	m_pSpheres = _pSpheres;
}

void iABoneThicknessMouseInteractor::OnLeftButtonDown()
{
	const int* pClickPos(GetInteractor()->GetEventPosition());

	m_pPicker->Pick(pClickPos[0], pClickPos[1], 0.0, m_pRenderer);

	const vtkIdType idPickedActor(m_pSpheres->IsItemPresent((vtkActor*)m_pPicker->GetActor()) - 1);

	if ((idPickedActor == m_pBoneThicknessTable->selected()) || (idPickedActor < 0))
	{
		m_pBoneThickness->setSelected(idPickedActor);
		m_pBoneThicknessChartBar->setSelected(idPickedActor);
	}
	else
	{
		m_pBoneThicknessTable->setSelected(idPickedActor);
	}

	vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}
