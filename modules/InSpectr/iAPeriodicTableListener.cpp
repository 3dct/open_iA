// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPeriodicTableListener.h"

#include "dlg_InSpectr.h"
#include "iAElementConstants.h"


iAElementSelectionListener::~iAElementSelectionListener()
{}

iAPeriodicTableListener::iAPeriodicTableListener(dlg_InSpectr* dlgXRF):
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
