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
#include <vtkCellPicker.h>
#include <vtkExtractGeometry.h>
#include <vtkIdTypeArray.h>
#include <vtkPlane.h>
#include <vtkPlanes.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkVertexGlyphFilter.h>

namespace
{
	int FontSize = 14;
	int TextMargin = 2;
	const char * SelectHelpText = "To select something, press 'r' while this window has focus.";
	const char * SelectModeText = "Selection Mode";
}

vtkStandardNewMacro(iASelectionInteractorStyle);

iASelectionInteractorStyle::iASelectionInteractorStyle():
	m_selectionProvider(nullptr),
	m_showModeActor(vtkSmartPointer<vtkTextActor>::New()),
	m_selectionMode(smDrag)
{
	m_showModeActor->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
	m_showModeActor->GetTextProperty()->SetBackgroundColor(1.0, 1.0, 1.0);
	m_showModeActor->GetTextProperty()->SetBackgroundOpacity(0.5);
	m_showModeActor->GetTextProperty()->SetFontSize(FontSize);
	m_showModeActor->SetPosition(TextMargin, TextMargin);
}

void iASelectionInteractorStyle::setSelectionProvider(iASelectionProvider *selectionProvider)
{
	m_selectionProvider = selectionProvider;
}
namespace
{
	// must match values in vtkInteractorStyleRubberBandPick!
	const int VTKISRBP_ORIENT = 0;
	const int VTKISRBP_SELECT = 1;
}

void iASelectionInteractorStyle::OnChar()
{
	vtkInteractorStyleRubberBandPick::OnChar();
	m_showModeActor->SetInput( this->CurrentMode == VTKISRBP_SELECT ? SelectModeText : "" );
	m_renWin->Render();
}

void iASelectionInteractorStyle::Pick()
{
	if (m_selectionMode != smDrag)
		return;
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

		if (!m_input.contains(resultID))
			continue;

		vtkSmartPointer<vtkExtractGeometry> extractGeometry = vtkSmartPointer<vtkExtractGeometry>::New();
		extractGeometry->SetImplicitFunction(frustum);
		extractGeometry->SetInputData(m_input[resultID]);
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

void iASelectionInteractorStyle::OnLeftButtonDown()
{
	vtkInteractorStyleRubberBandPick::OnLeftButtonDown();
	if (m_selectionMode != smClick)
	{
		return;
	}
	if (!m_cellRenderer)
	{
		DEBUG_LOG("Cell renderer not set!");
		return;
	}

	// Get the location of the click (in window coordinates)
	int* pos = this->GetInteractor()->GetEventPosition();

	auto picker = vtkSmartPointer<vtkCellPicker>::New();
	picker->SetTolerance(0.0005);

	// Pick from this location.
	picker->Pick(pos[0], pos[1], 0, m_cellRenderer);

	double* worldPosition = picker->GetPickPosition();
	DEBUG_LOG(QString("Cell id is: %1").arg(picker->GetCellId()));

	if(picker->GetCellId() != -1)
	{
		size_t objectID = (picker->GetCellId()) / 14;
		DEBUG_LOG(QString("Pick position is (%1, %2, %3), object: %4")
				  .arg(worldPosition[0]).arg(worldPosition[1])
				  .arg(worldPosition[2]).arg(objectID));
		for (int i=0; i<m_selectionProvider->selection().size(); ++i)
			m_selectionProvider->selection()[i].clear();
		m_selectionProvider->selection()[lastResultID].push_back(objectID);
		emit selectionChanged();
	}

}

void iASelectionInteractorStyle::addInput(size_t resultID, vtkSmartPointer<vtkPolyData> points)
{
	lastResultID = resultID;
	m_input.insert(resultID, points);
}

void iASelectionInteractorStyle::removeInput(size_t resultID)
{
	m_input.remove(resultID);
}

void iASelectionInteractorStyle::assignToRenderWindow(vtkSmartPointer<vtkRenderWindow> renWin)
{
	vtkSmartPointer<vtkAreaPicker> areaPicker = vtkSmartPointer<vtkAreaPicker>::New();
	m_renWin = renWin;
	m_renWin->GetInteractor()->SetPicker(areaPicker);
	m_renWin->GetInteractor()->SetInteractorStyle(this);
	m_renWin->GetRenderers()->GetFirstRenderer()->AddActor2D(m_showModeActor);
	m_showModeActor->SetInput("To select something, press 'r' while this window has focus.");
}

void iASelectionInteractorStyle::setSelectionMode(SelectionMode mode)
{
	m_selectionMode = mode;
	CurrentMode = m_selectionMode == smDrag ? VTKISRBP_SELECT : VTKISRBP_ORIENT;
}

void iASelectionInteractorStyle::setRenderer(vtkRenderer* renderer)
{
	m_cellRenderer = renderer;
}
