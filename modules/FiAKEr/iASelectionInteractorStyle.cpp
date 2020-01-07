/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
	const char * NavigationModeText = "Navigation Mode (click 's' to switch to Selection Mode)";
	const char * SelectModeText = "Selection Mode";
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
		m_showModeActor->SetInput(m_interactionMode == imNavigate ? NavigationModeText : SelectModeText);
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
	int* size = this->Interactor->GetRenderWindow()->GetSize();
	int min[2], max[2];
	min[0] = m_startPos[0] <= m_endPos[0] ?
		m_startPos[0] : m_endPos[0];
	if (min[0] < 0) { min[0] = 0; }
	if (min[0] >= size[0]) { min[0] = size[0] - 2; }

	min[1] = m_startPos[1] <= m_endPos[1] ?
		m_startPos[1] : m_endPos[1];
	if (min[1] < 0) { min[1] = 0; }
	if (min[1] >= size[1]) { min[1] = size[1] - 2; }

	max[0] = m_endPos[0] > m_startPos[0] ?
		m_endPos[0] : m_startPos[0];
	if (max[0] < 0) { max[0] = 0; }
	if (max[0] >= size[0]) { max[0] = size[0] - 2; }

	max[1] = m_endPos[1] > m_startPos[1] ?
		m_endPos[1] : m_startPos[1];
	if (max[1] < 0) { max[1] = 0; }
	if (max[1] >= size[1]) { max[1] = size[1] - 2; }

	rbcenter[0] = (min[0] + max[0]) / 2.0;
	rbcenter[1] = (min[1] + max[1]) / 2.0;
	rbcenter[2] = 0;

	if (this->State == VTKIS_NONE)
	{
		//tell the RenderWindowInteractor's picker to make it happen
		vtkRenderWindowInteractor* rwi = this->Interactor;

		vtkAssemblyPath* path = nullptr;
		rwi->StartPickCallback();
		vtkAbstractPropPicker* picker =
			vtkAbstractPropPicker::SafeDownCast(rwi->GetPicker());
		if (picker != nullptr)
		{
			vtkAreaPicker* areaPicker = vtkAreaPicker::SafeDownCast(picker);
			if (areaPicker != nullptr)
			{
				areaPicker->AreaPick(min[0], min[1], max[0], max[1],
					this->CurrentRenderer);
			}
			else
			{
				picker->Pick(rbcenter[0], rbcenter[1],
					0.0, this->CurrentRenderer);
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
			continue;

		for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); i++)
		{
			size_t objID = ids->GetValue(i);
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
		m_pixelArray->SetNumberOfComponents(4);
		int* size = renWin->GetSize();
		m_pixelArray->SetNumberOfTuples(size[0] * size[1]);

		renWin->GetRGBACharPixelData(0, 0, size[0] - 1, size[1] - 1, 1, m_pixelArray);

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
			for (int i = 0; i < m_selectionProvider->selection().size(); ++i)
				m_selectionProvider->selection()[i].clear();
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

	m_endPos[0] = this->Interactor->GetEventPosition()[0];
	m_endPos[1] = this->Interactor->GetEventPosition()[1];
	int* size = this->Interactor->GetRenderWindow()->GetSize();
	if (m_endPos[0] > (size[0] - 1))
	{
		m_endPos[0] = size[0] - 1;
	}
	if (m_endPos[0] < 0)
	{
		m_endPos[0] = 0;
	}
	if (m_endPos[1] > (size[1] - 1))
	{
		m_endPos[1] = size[1] - 1;
	}
	if (m_endPos[1] < 0)
	{
		m_endPos[1] = 0;
	}
	redrawRubberBand();
}

//--------------------------------------------------------------------------
void iASelectionInteractorStyle::OnLeftButtonUp()
{
	if (m_interactionMode != imSelect)
	{
		//if not in rubber band mode,  let the parent class handle it
		this->Superclass::OnLeftButtonUp();
		return;
	}

	if (!this->Interactor || !m_moving)
	{
		return;
	}

	//otherwise record the rubber band end coordinate and then fire off a pick
	if ((m_startPos[0] != m_endPos[0])
		|| (m_startPos[1] != m_endPos[1]))
	{
		pick();
	}
	m_moving = false;
}

//--------------------------------------------------------------------------
void iASelectionInteractorStyle::redrawRubberBand()
{
	//update the rubber band on the screen
	int* size = this->Interactor->GetRenderWindow()->GetSize();

	vtkUnsignedCharArray* tmpPixelArray = vtkUnsignedCharArray::New();
	tmpPixelArray->DeepCopy(m_pixelArray);
	unsigned char* pixels = tmpPixelArray->GetPointer(0);

	int min[2], max[2];

	min[0] = m_startPos[0] <= m_endPos[0] ?
		m_startPos[0] : m_endPos[0];
	if (min[0] < 0) { min[0] = 0; }
	if (min[0] >= size[0]) { min[0] = size[0] - 1; }

	min[1] = m_startPos[1] <= m_endPos[1] ?
		m_startPos[1] : m_endPos[1];
	if (min[1] < 0) { min[1] = 0; }
	if (min[1] >= size[1]) { min[1] = size[1] - 1; }

	max[0] = m_endPos[0] > m_startPos[0] ?
		m_endPos[0] : m_startPos[0];
	if (max[0] < 0) { max[0] = 0; }
	if (max[0] >= size[0]) { max[0] = size[0] - 1; }

	max[1] = m_endPos[1] > m_startPos[1] ?
		m_endPos[1] : m_startPos[1];
	if (max[1] < 0) { max[1] = 0; }
	if (max[1] >= size[1]) { max[1] = size[1] - 1; }

	int i;
	for (i = min[0]; i <= max[0]; i++)
	{
		pixels[4 * (min[1] * size[0] + i)] = 255 ^ pixels[4 * (min[1] * size[0] + i)];
		pixels[4 * (min[1] * size[0] + i) + 1] = 255 ^ pixels[4 * (min[1] * size[0] + i) + 1];
		pixels[4 * (min[1] * size[0] + i) + 2] = 255 ^ pixels[4 * (min[1] * size[0] + i) + 2];
		pixels[4 * (max[1] * size[0] + i)] = 255 ^ pixels[4 * (max[1] * size[0] + i)];
		pixels[4 * (max[1] * size[0] + i) + 1] = 255 ^ pixels[4 * (max[1] * size[0] + i) + 1];
		pixels[4 * (max[1] * size[0] + i) + 2] = 255 ^ pixels[4 * (max[1] * size[0] + i) + 2];
	}
	for (i = min[1] + 1; i < max[1]; i++)
	{
		pixels[4 * (i * size[0] + min[0])] = 255 ^ pixels[4 * (i * size[0] + min[0])];
		pixels[4 * (i * size[0] + min[0]) + 1] = 255 ^ pixels[4 * (i * size[0] + min[0]) + 1];
		pixels[4 * (i * size[0] + min[0]) + 2] = 255 ^ pixels[4 * (i * size[0] + min[0]) + 2];
		pixels[4 * (i * size[0] + max[0])] = 255 ^ pixels[4 * (i * size[0] + max[0])];
		pixels[4 * (i * size[0] + max[0]) + 1] = 255 ^ pixels[4 * (i * size[0] + max[0]) + 1];
		pixels[4 * (i * size[0] + max[0]) + 2] = 255 ^ pixels[4 * (i * size[0] + max[0]) + 2];
	}

	this->Interactor->GetRenderWindow()->SetRGBACharPixelData(0, 0, size[0] - 1, size[1] - 1, pixels, 0);
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
	m_showModeActor->SetInput(NavigationModeText);
}

void iASelectionInteractorStyle::setSelectionMode(SelectionMode mode)
{
	m_selectionMode = mode;
}

void iASelectionInteractorStyle::setRenderer(vtkRenderer* renderer)
{
	m_cellRenderer = renderer;
}
