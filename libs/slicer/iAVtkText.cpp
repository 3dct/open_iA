/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVtkText.h"

#include <vtkActor2D.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>

vtkStandardNewMacro(iAVtkText);

iAVtkText::iAVtkText():
	m_textMapper(vtkSmartPointer<vtkTextMapper>::New()),
	m_actor(vtkSmartPointer<vtkActor2D>::New())
{
	m_textMapper->SetInput("");

	vtkTextProperty* pProperty = m_textMapper->GetTextProperty();
	pProperty->SetBold(1);
	pProperty->SetFontSize(10);
	pProperty->SetShadow(1);
	pProperty->SetFontFamily(VTK_COURIER);
	pProperty->SetColor(0.5, 0.65, 0.86);
	pProperty->SetJustification(VTK_TEXT_LEFT);
	pProperty->SetVerticalJustification(VTK_TEXT_CENTERED);

	m_actor->SetMapper(m_textMapper);
}

void iAVtkText::addToScene(vtkRenderer* renderer)
{
	renderer->AddActor(m_actor);
	m_textMapper->GetTextProperty()->SetOpacity(1.0);
}

void iAVtkText::setPosition(double x, double y)
{
	m_actor->SetPosition(x, y);
}

void iAVtkText::setText(const char* text)
{
	m_textMapper->SetInput(text);
}

void iAVtkText::setFontSize(int fontSize)
{
	m_textMapper->GetTextProperty()->SetFontSize(fontSize);
}

void iAVtkText::show(bool show)
{
	m_actor->SetVisibility(show);
}

bool iAVtkText::isShown() const
{
	return m_actor->GetVisibility();
}
