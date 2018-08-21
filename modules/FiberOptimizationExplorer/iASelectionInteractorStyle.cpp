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

#include <set>

vtkStandardNewMacro(iASelectionInteractorStyle);

void iASelectionInteractorStyle::OnLeftButtonUp()
{
	vtkInteractorStyleRubberBandPick::OnLeftButtonUp();

	if (!m_points)
		return;

	vtkPlanes* frustum = static_cast<vtkAreaPicker*>(GetInteractor()->GetPicker())->GetFrustum();

	vtkSmartPointer<vtkExtractGeometry> extractGeometry = vtkSmartPointer<vtkExtractGeometry>::New();
	extractGeometry->SetImplicitFunction(frustum);
	extractGeometry->SetInputData(m_points);
	extractGeometry->Update();

	vtkSmartPointer<vtkVertexGlyphFilter> glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
	glyphFilter->SetInputConnection(extractGeometry->GetOutputPort());
	glyphFilter->Update();

	vtkPolyData* selected = glyphFilter->GetOutput();
	vtkPointData* pointData = selected->GetPointData();
	vtkIdTypeArray* ids = vtkIdTypeArray::SafeDownCast(pointData->GetArray("OriginalIds"));
	if (!ids)
		return;

	std::set<size_t> selset;
	for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); i++)
	{
		selset.insert(ids->GetValue(i) / 2);
	}
	std::vector<size_t> selection;
	std::copy(selset.begin(), selset.end(), std::back_inserter(selection));
	emit selectionChanged(selection);
}


void iASelectionInteractorStyle::setInput(vtkSmartPointer<vtkPolyData> points)
{
	m_points = points;
}

void iASelectionInteractorStyle::assignToRenderWindow(vtkSmartPointer<vtkRenderWindow> renWin)
{
	vtkSmartPointer<vtkAreaPicker> areaPicker = vtkSmartPointer<vtkAreaPicker>::New();
	renWin->GetInteractor()->SetPicker(areaPicker);
	renWin->GetInteractor()->SetInteractorStyle(this);
}