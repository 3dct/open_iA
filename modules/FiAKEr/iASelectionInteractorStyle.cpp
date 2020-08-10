/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAMathUtility.h"

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
#include <vtkUnsignedCharArray.h>
#include <vtkVertexGlyphFilter.h>

namespace
{
	int FontSize = 14;
	int TextMargin = 2;
	QString const NavigationModeText("Navigation Mode (click 's' to switch to Selection Mode)");
	QString const SelectModeText("Selection Mode (%1)");
	QString const DragSelectionMode("Drag Rectangle");
	QString const ClickSelectionMode("Click Fiber");

	void computeMinMax(int minVal[2], int maxVal[2], int const startPos[2], int const endPos[2], int const size[2])
	{
		minVal[0] = clamp(0, size[0] - 1, startPos[0] <= endPos[0] ? startPos[0] : endPos[0]);
		minVal[1] = clamp(0, size[1] - 1, startPos[1] <= endPos[1] ? startPos[1] : endPos[1]);
		maxVal[0] = clamp(0, size[0] - 1, endPos[0] > startPos[0] ? endPos[0] : startPos[0]);
		maxVal[1] = clamp(0, size[1] - 1, endPos[1] > startPos[1] ? endPos[1] : startPos[1]);
	}
}

iASelectionProvider::~iASelectionProvider()
{}

vtkStandardNewMacro(iASelectionInteractorStyle);

iASelectionInteractorStyle::iASelectionInteractorStyle():
	m_selectionProvider(nullptr),
	m_showModeActor(vtkSmartPointer<vtkTextActor>::New()),
	m_interactionMode(imNavigate),
	m_selectionMode(smDrag),
	m_moving(false),
	m_pixelArray(vtkSmartPointer<vtkUnsignedCharArray>::New())
{
	m_startPos[0] = m_startPos[1] = m_endPos[0] = m_endPos[1] = 0;

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

void iASelectionInteractorStyle::updateModeLabel()
{
	QString text = m_interactionMode == imNavigate ? NavigationModeText :
		SelectModeText.arg(m_selectionMode == smDrag ? DragSelectionMode : ClickSelectionMode);
	m_showModeActor->SetInput(text.toStdString().c_str());
}

void iASelectionInteractorStyle::OnChar()
{
	switch (this->Interactor->GetKeyCode())
	{
	case 's':
	case 'S':
		//r toggles the rubber band selection mode for mouse button 1
		if (m_interactionMode == imNavigate)
		{
			m_interactionMode = imSelect;
		}
		else
		{
			m_interactionMode = imNavigate;
		}
		updateModeLabel();
		break;
	case 'p':
	case 'P':
	{
		vtkRenderWindowInteractor* rwi = this->Interactor;
		int* eventPos = rwi->GetEventPosition();
		this->FindPokedRenderer(eventPos[0], eventPos[1]);
		m_startPos[0] = eventPos[0];
		m_startPos[1] = eventPos[1];
		m_endPos[0] = eventPos[0];
		m_endPos[1] = eventPos[1];
		pick();
		break;
	}
	default:
		this->Superclass::OnChar();
	}
	m_renWin->Render();
}

void iASelectionInteractorStyle::pick()
{
	if (m_selectionMode != smDrag)
	{
		return;
	}
	if (!m_selectionProvider)
	{
		DEBUG_LOG("No selection provider given!");
		return;
	}

	//find rubber band lower left, upper right and center
	double rbcenter[3];
	int const* size = this->Interactor->GetRenderWindow()->GetSize();

	int minVal[2], maxVal[2];
	computeMinMax(minVal, maxVal, m_startPos, m_endPos, size);

	rbcenter[0] = (minVal[0] + maxVal[0]) / 2.0;
	rbcenter[1] = (minVal[1] + maxVal[1]) / 2.0;
	rbcenter[2] = 0;

	if (this->State == VTKIS_NONE)
	{
		//tell the RenderWindowInteractor's picker to make it happen
		vtkRenderWindowInteractor* rwi = this->Interactor;

		vtkAssemblyPath* path = nullptr;
		rwi->StartPickCallback();
		vtkAbstractPropPicker* picker =	vtkAbstractPropPicker::SafeDownCast(rwi->GetPicker());
		if (picker != nullptr)
		{
			vtkAreaPicker* areaPicker = vtkAreaPicker::SafeDownCast(picker);
			if (areaPicker != nullptr)
			{
				areaPicker->AreaPick(minVal[0], minVal[1], maxVal[0], maxVal[1],
					this->CurrentRenderer);
			}
			else
			{
				picker->Pick(rbcenter[0], rbcenter[1], 0.0, this->CurrentRenderer);
			}
			path = picker->GetPath();
		}
		if (path == nullptr)
		{
			this->HighlightProp(nullptr);
			this->PropPicked = 0;
		}
		else
		{
			//highlight the one prop that the picker saved in the path
			//this->HighlightProp(path->GetFirstNode()->GetViewProp());
			this->PropPicked = 1;
		}
		rwi->EndPickCallback();
	}

	this->Interactor->Render();

	vtkPlanes* frustum = static_cast<vtkAreaPicker*>(GetInteractor()->GetPicker())->GetFrustum();

	for (size_t resultID=0; resultID < m_selectionProvider->selection().size(); ++resultID)
	{
		auto& resultSel = m_selectionProvider->selection()[resultID];

		if (!GetInteractor()->GetAltKey() && !GetInteractor()->GetShiftKey())
		{
			resultSel.clear();
		}

		if (!m_input.contains(resultID))
		{
			continue;
		}

		vtkSmartPointer<vtkExtractGeometry> extractGeometry = vtkSmartPointer<vtkExtractGeometry>::New();
		extractGeometry->SetImplicitFunction(frustum);
		extractGeometry->SetInputData(m_input[resultID].first);
		extractGeometry->Update();

		vtkSmartPointer<vtkVertexGlyphFilter> glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
		glyphFilter->SetInputConnection(extractGeometry->GetOutputPort());
		glyphFilter->Update();

		vtkPolyData* selected = glyphFilter->GetOutput();
		vtkPointData* pointData = selected->GetPointData();
		vtkIdTypeArray* ids = vtkIdTypeArray::SafeDownCast(pointData->GetArray("OriginalIds"));
		if (!ids)
		{
			continue;
		}
		for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); i++)
		{
			size_t objID = ids->GetValue(i);
			auto it = std::find(resultSel.begin(), resultSel.end(), objID);
			if (it != resultSel.end() && GetInteractor()->GetAltKey())
			{
				resultSel.erase(it);
			}
			else if (it == resultSel.end() && (!GetInteractor()->GetAltKey() || GetInteractor()->GetShiftKey()))
			{
				resultSel.push_back(objID);
			}
		}
	}
	emit selectionChanged();
}

void iASelectionInteractorStyle::OnLeftButtonDown()
{
	if (m_interactionMode != imSelect)
	{
		//if not in rubber band mode, let the parent class handle it
		this->Superclass::OnLeftButtonDown();
		return;
	}

	if (!this->Interactor)
	{
		return;
	}

	//otherwise record the rubber band starting coordinate
	if (m_selectionMode == smDrag)
	{
		m_moving = true;

		vtkRenderWindow* renWin = this->Interactor->GetRenderWindow();

		m_startPos[0] = this->Interactor->GetEventPosition()[0];
		m_startPos[1] = this->Interactor->GetEventPosition()[1];
		m_endPos[0] = m_startPos[0];
		m_endPos[1] = m_startPos[1];

		m_pixelArray->Initialize();
		m_pixelArray->SetNumberOfComponents(3);
		int const* size = renWin->GetSize();
		m_pixelArray->SetNumberOfTuples(size[0] * size[1]);

		renWin->GetPixelData(0, 0, size[0] - 1, size[1] - 1, 1, m_pixelArray);

		this->FindPokedRenderer(m_startPos[0], m_startPos[1]);

	}
	else // smClick
	{
		if (!m_cellRenderer)
		{
			DEBUG_LOG("Cell renderer not set!");
			return;
		}

		// Get the location of the click (in window coordinates)
		int* pos = this->GetInteractor()->GetEventPosition();

		auto picker = vtkSmartPointer<vtkCellPicker>::New();
		picker->SetTolerance(0.0005);
		picker->Pick(pos[0], pos[1], 0, m_cellRenderer);
		//double* worldPosition = picker->GetPickPosition();

		if (picker->GetCellId() != -1)
		{
			size_t pickedResultID = std::numeric_limits<size_t>::max();
			for (size_t curResultID : m_input.keys())
			{
				if (m_input[curResultID].second.GetPointer() == picker->GetActor())
				{
					pickedResultID = curResultID;
					break;
				}
			}
			if (pickedResultID == std::numeric_limits<size_t>::max())
			{
				DEBUG_LOG("Could not find picked result.");
				return;
			}
			size_t objectID = (picker->GetCellId()) / 14;
			for (size_t i = 0; i < m_selectionProvider->selection().size(); ++i)
			{
				m_selectionProvider->selection()[i].clear();
			}
			m_selectionProvider->selection()[pickedResultID].push_back(objectID);
			emit selectionChanged();
		}
	}
}

void iASelectionInteractorStyle::OnMouseMove()
{
	if (m_interactionMode != imSelect)
	{
		//if not in rubber band mode,  let the parent class handle it
		this->Superclass::OnMouseMove();
		return;
	}

	if (!this->Interactor || !m_moving)
	{
		return;
	}

	int const* size = this->Interactor->GetRenderWindow()->GetSize();
	m_endPos[0] = clamp(0, size[0] - 1, this->Interactor->GetEventPosition()[0]);
	m_endPos[1] = clamp(0, size[1] - 1, this->Interactor->GetEventPosition()[1]);
	redrawRubberBand();
}

void iASelectionInteractorStyle::OnLeftButtonUp()
{
	if (m_interactionMode != imSelect)
	{
		//if not in rubber band mode, let the parent class handle it
		this->Superclass::OnLeftButtonUp();
		return;
	}

	if (!this->Interactor || !m_moving)
	{
		return;
	}

	//otherwise record the rubber band end coordinate and then fire off a pick
	if (m_startPos[0] != m_endPos[0] || m_startPos[1] != m_endPos[1])
	{
		pick();
	}
	m_moving = false;
}

void iASelectionInteractorStyle::redrawRubberBand()
{
	int const* size = this->Interactor->GetRenderWindow()->GetSize();

	vtkUnsignedCharArray* tmpPixelArray = vtkUnsignedCharArray::New();
	tmpPixelArray->DeepCopy(m_pixelArray);
	unsigned char* pixels = tmpPixelArray->GetPointer(0);

	int minVal[2], maxVal[2];
	computeMinMax(minVal, maxVal, m_startPos, m_endPos, size);

	for (int i = minVal[0]; i <= maxVal[0]; i++)
	{
		for (int c = 0; c < 3; ++c)
		{
			int minIdx = 3 * (minVal[1] * size[0] + i) + c;
			int maxIdx = 3 * (maxVal[1] * size[0] + i) + c;
			pixels[minIdx] = 255 ^ pixels[minIdx];
			pixels[maxIdx] = 255 ^ pixels[maxIdx];
		}
	}
	for (int i = minVal[1] + 1; i < maxVal[1]; i++)
	{
		for (int c = 0; c < 3; ++c)
		{
			int minIdx = 3 * (i * size[0] + minVal[0]) + c;
			int maxIdx = 3 * (i * size[0] + maxVal[0]) + c;
			pixels[minIdx] = 255 ^ pixels[minIdx];
			pixels[maxIdx] = 255 ^ pixels[maxIdx];
		}
	}

	this->Interactor->GetRenderWindow()->SetPixelData(0, 0, size[0] - 1, size[1] - 1, pixels, 1);
	this->Interactor->GetRenderWindow()->Frame();

	tmpPixelArray->Delete();
}

void iASelectionInteractorStyle::addInput(size_t resultID, vtkSmartPointer<vtkPolyData> points, vtkSmartPointer<vtkActor> actor)
{
	m_input.insert(resultID, std::make_pair(points, actor));
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
	updateModeLabel();
}

void iASelectionInteractorStyle::setSelectionMode(SelectionMode mode)
{
	m_selectionMode = mode;
	updateModeLabel();
}

void iASelectionInteractorStyle::setRenderer(vtkRenderer* renderer)
{
	m_cellRenderer = renderer;
}
