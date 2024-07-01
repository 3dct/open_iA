// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAInSpectrModuleInterface.h"

#include "iAInSpectrTool.h"

#include <iAToolHelper.h>    // for addToolAction

void iAInSpectrModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	addToolAction<iAInSpectrTool>(m_mainWnd, m_mainWnd->toolsMenu());
}
