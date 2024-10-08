// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemoteModuleInterface.h"

#include "iAPlaneSliceTool.h"
#include "iARemoteTool.h"
#include "iAUnityWebsocketServerTool.h"

#include <iAMainWindow.h>
#include <iAToolHelper.h>    // for addToolToActiveMdiChild, addToolAction
#include <iAToolRegistry.h>

void iARemoteModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	iAToolRegistry::addTool(iAPlaneSliceTool::Name, createTool<iAPlaneSliceTool>);
	iAToolRegistry::addTool(iAUnityWebsocketServerTool::Name, createTool<iAUnityWebsocketServerTool>);
	iAToolRegistry::addTool(iARemoteTool::Name, createTool<iARemoteTool>);

	auto remoteMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Remote"), false);
	addToolAction<iAPlaneSliceTool>(m_mainWnd, m_mainWnd->toolsMenu());
	addToolAction<iAUnityWebsocketServerTool>(m_mainWnd, remoteMenu);
	addToolAction<iARemoteTool>(m_mainWnd, remoteMenu);
}
