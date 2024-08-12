// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemoteModuleInterface.h"

#include "iAPlaneSliceTool.h"
#include "iARemoteTool.h"
#include "iAUnityWebsocketServerTool.h"

#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAToolHelper.h>    // for addToolToActiveMdiChild, addToolAction
#include <iAToolRegistry.h>

#include <iALog.h>

void iARemoteModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	iAToolRegistry::addTool(iAPlaneSliceTool::Name, iAPlaneSliceTool::create);
	iAToolRegistry::addTool(iAUnityWebsocketServerTool::Name, iAUnityWebsocketServerTool::create);
	iAToolRegistry::addTool(iARemoteTool::Name, createTool<iARemoteTool>);

	auto remoteMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Remote"), false);
	addToolAction<iAPlaneSliceTool>(m_mainWnd, m_mainWnd->toolsMenu());
	addToolAction<iAUnityWebsocketServerTool>(m_mainWnd, remoteMenu);
	addToolAction<iARemoteTool>(m_mainWnd, remoteMenu);
}
