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

/*=========================================================================

Program: DevSample_PickPixelValue

Copyright (c) Mark Wyszomierski
All rights reserved.
See copyright.txt or http://www.devsample.org/copyright.txt for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the above copyright notice for more information.

=========================================================================*/
#include "iAWrapperText.h"

#include <vtkActor2D.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>

vtkStandardNewMacro(iAWrapperText);


iAWrapperText::iAWrapperText()
{
	m_TextMapper = vtkTextMapper::New();
	m_TextMapper->SetInput("");

	vtkTextProperty *pProperty = m_TextMapper->GetTextProperty();
	pProperty->SetBold(1);
	pProperty->SetFontSize(10);
	pProperty->SetShadow(1);
	pProperty->SetFontFamily(VTK_COURIER);
	pProperty->SetColor(0.5, 0.65, 0.86);
	pProperty->SetJustification(VTK_TEXT_CENTERED);
	pProperty->SetVerticalJustification(VTK_TEXT_CENTERED);
	m_nPositionType = POS_CENTER;
	m_cxWin = 0;
	m_cyWin = 0;

	m_Actor = vtkActor2D::New();
	m_Actor->SetMapper(m_TextMapper);
}


iAWrapperText::~iAWrapperText()
{
	m_TextMapper->Delete();
	m_Actor->Delete();
}


void iAWrapperText::AddToScene(vtkRenderer *pParentRenderer)
{
	pParentRenderer->AddActor(m_Actor);
	Show(0);
	Show(1);
}


void iAWrapperText::SetParentWindowSize(int cxWin, int cyWin)
{
	m_cxWin = cxWin;
	m_cyWin = cyWin;
	SetPosition(m_nPositionType);
}


void iAWrapperText::SetText(const char *pszText)
{
	m_TextMapper->SetInput(pszText);
}


void iAWrapperText::Show(int bShow)
{
	m_TextMapper->GetTextProperty()->SetOpacity(bShow);
}


void iAWrapperText::SetPosition(int nPosition)
{
	m_nPositionType = nPosition;
	switch (m_nPositionType) {
	case POS_CENTER:
		m_Actor->SetPosition(m_cxWin/2, m_cyWin/2);
		m_TextMapper->GetTextProperty()->SetJustification(VTK_TEXT_CENTERED);
		break;
	case POS_UPPER_LEFT:
		m_Actor->SetPosition(20, m_cyWin-20);
		m_TextMapper->GetTextProperty()->SetJustification(VTK_TEXT_LEFT);
		break;
	case POS_UPPER_RIGHT:
		m_Actor->SetPosition(m_cxWin-20, m_cyWin-20);
		m_TextMapper->GetTextProperty()->SetJustification(VTK_TEXT_RIGHT);
		break;
	case POS_LOWER_RIGHT:
		m_Actor->SetPosition(m_cxWin-20, 20);
		m_TextMapper->GetTextProperty()->SetJustification(VTK_TEXT_RIGHT);
		break;
	case POS_LOWER_LEFT:
		m_Actor->SetPosition(20, 20);
		m_TextMapper->GetTextProperty()->SetJustification(VTK_TEXT_LEFT);
		break;
	}
}
