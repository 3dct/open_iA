// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iAVolumePlayerModuleInterface.h>

#include "iAVolumePlayerWidget.h"

#include <iAToolHelper.h>

#include <QMenu>

void iAVolumePlayerModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	addToolAction<iAVolumePlayerTool>(m_mainWnd, m_mainWnd->toolsMenu());
}
