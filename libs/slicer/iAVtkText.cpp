// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
