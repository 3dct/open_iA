// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAElementSelectionListener.h"

class dlg_InSpectr;

class iAPeriodicTableListener: public iAElementSelectionListener
{
private:
	dlg_InSpectr* m_dlgXRF;
public:
	iAPeriodicTableListener(dlg_InSpectr* dlgXRF);
	virtual void ElementEnter(int elementIdx);
	virtual void ElementLeave(int elementIdx);
};
