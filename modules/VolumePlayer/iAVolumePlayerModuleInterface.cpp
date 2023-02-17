// Copyright 2016-2023, the open_iA contributors
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
	QString toolName("VolumePlayer");
	m_mainWnd->toolsMenu()->addAction(toolName,
		[this, toolName] { addToolToActiveMdiChild<iAVolumePlayerTool>(toolName, m_mainWnd); });
}
