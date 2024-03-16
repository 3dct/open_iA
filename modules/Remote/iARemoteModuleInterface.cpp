// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemoteModuleInterface.h"

#include "iAOpenXRTrackerServerTool.h"
#include "iAPlaneSliceTool.h"
#include "iARemoteTool.h"
#include "iAUnityWebsocketServerTool.h"

#include <iALog.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAToolHelper.h>    // for addToolToActiveMdiChild, addToolAction

void iARemoteModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}

	auto submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Remote"), false);

	QAction* actionRemote = new QAction(tr("Remote Render Server"), m_mainWnd);
	connect(actionRemote, &QAction::triggered, this,[this]()
		{
			// cannot start remote server twice - hard-coded websocket ports!
			for (auto c : m_mainWnd->mdiChildList())
			{
				if (getTool<iARemoteTool>(c))
				{
					LOG(lvlWarn, "Remote render server already running!");
					return;
				}
			}
			// TODO: - find way to inject websocket port into served html? we could modify served file on the fly...
			//       - then we would need to modify above check to only check for current child (no sense in serving same child twice)
			addToolToActiveMdiChild<iARemoteTool>(iARemoteTool::Name, m_mainWnd);
		});
	m_mainWnd->makeActionChildDependent(actionRemote);
	addToMenuSorted(submenu, actionRemote);

	addToolAction<iAOpenXRTrackerServerTool>(m_mainWnd, submenu);
	addToolAction<iAPlaneSliceTool>(m_mainWnd, submenu);
	addToolAction<iAUnityWebsocketServerTool>(m_mainWnd, submenu);
}
