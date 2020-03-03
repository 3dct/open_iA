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
#include "iAPeriodicTableListener.h"

#include "dlg_XRF.h"
#include "iAElementConstants.h"


iAElementSelectionListener::~iAElementSelectionListener()
{}

iAPeriodicTableListener::iAPeriodicTableListener(dlg_XRF* dlgXRF):
	m_dlgXRF(dlgXRF)
{}

void iAPeriodicTableListener::ElementEnter(int elementIdx)
{
	if (m_dlgXRF->IsElementSelected(elementIdx))
	{
		return;
	}
	if (m_dlgXRF->ShowElementLines())
	{
		m_dlgXRF->AddElementLine(PeriodicTable::elements[elementIdx].shortname.c_str());
	}
	if (m_dlgXRF->ShowReferenceSpectra())
	{
		m_dlgXRF->AddReferenceSpectrum(m_dlgXRF->GetModelIdx(elementIdx));
	}
}

void iAPeriodicTableListener::ElementLeave(int elementIdx)
{
	if (m_dlgXRF->IsElementSelected(elementIdx))
	{
		return;
	}
	m_dlgXRF->RemoveElementLine(PeriodicTable::elements[elementIdx].shortname.c_str());
	m_dlgXRF->RemoveReferenceSpectrum(m_dlgXRF->GetModelIdx(elementIdx));
}