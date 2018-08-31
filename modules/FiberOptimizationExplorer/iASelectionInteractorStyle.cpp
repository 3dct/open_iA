/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iASelectionInteractorStyle.h"

#include "iAConsole.h"

#include <vtkAreaPicker.h>
#include <vtkExtractGeometry.h>
#include <vtkIdTypeArray.h>
#include <vtkPlane.h>
#include <vtkPlanes.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVertexGlyphFilter.h>

vtkStandardNewMacro(iASelectionInteractorStyle);

iASelectionInteractorStyle::iASelectionInteractorStyle():
	m_selectionProvider(nullptr)
{}

void iASelectionInteractorStyle::setSelectionProvider(iASelectionProvider *selectionProvider)
{
	m_selectionProvider = selectionProvider;
}

void iASelectionInteractorStyle::Pick()
{
	if (!m_selectionProvider)
	{
		DEBUG_LOG("No selection provider given!");
		return;
	}
	vtkInteractorStyleRubberBandPick::Pick();
	vtkPlanes* frustum = static_cast<vtkAreaPicker*>(GetInteractor()->GetPicker())->GetFrustum();

	for (size_t resultID=0; resultID < m_selectionProvider->selection().size(); ++resultID)
	{
		auto& resultSel = m_selectionProvider->selection()[resultID];

		if (!GetInteractor()->GetAltKey() && !GetInteractor()->GetShiftKey())
			resultSel.clear();

		if (!m_resultPoints.contains(resultID))
			continue;

		vtkSmartPointer<vtkExtractGeometry> extractGeometry = vtkSmartPointer<vtkExtractGeometry>::New();
		extractGeometry->SetImplicitFunction(frustum);
		extractGeometry->SetInputData(m_resultPoints[resultID]);
		extractGeometry->Update();

		vtkSmartPointer<vtkVertexGlyphFilter> glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
		glyphFilter->SetInputConnection(extractGeometry->GetOutputPort());
		glyphFilter->Update();

		vtkPolyData* selected = glyphFilter->GetOutput();
		vtkPointData* pointData = selected->GetPointData();
		vtkIdTypeArray* ids = vtkIdTypeArray::SafeDownCast(pointData->GetArray("OriginalIds"));
		if (!ids)
			continue;

		for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); i++)
		{
			size_t objID = ids->GetValue(i) / 2;
			auto it = std::find(resultSel.begin(), resultSel.end(), objID);
			if (it != resultSel.end() && GetInteractor()->GetAltKey())
				resultSel.erase( it );
			else if (it == resultSel.end() && (!GetInteractor()->GetAltKey() || GetInteractor()->GetShiftKey()))
				resultSel.push_back(objID);
		}
	}
	emit selectionChanged();
}

void iASelectionInteractorStyle::addInput(size_t resultID, vtkSmartPointer<vtkPolyData> points)
{
	m_resultPoints.insert(resultID, points);
}

void iASelectionInteractorStyle::removeInput(size_t resultID)
{
	m_resultPoints.remove(resultID);
}

void iASelectionInteractorStyle::assignToRenderWindow(vtkSmartPointer<vtkRenderWindow> renWin)
{
	vtkSmartPointer<vtkAreaPicker> areaPicker = vtkSmartPointer<vtkAreaPicker>::New();
	renWin->GetInteractor()->SetPicker(areaPicker);
	renWin->GetInteractor()->SetInteractorStyle(this);
}
