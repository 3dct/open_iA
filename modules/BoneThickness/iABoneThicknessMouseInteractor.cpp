/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
