// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALabellingModuleInterface.h"

#include "iAAnnotationTool.h"
#include "iALabellingTool.h"

#include <iAMainWindow.h>
#include <iAToolHelper.h>    // for addToolAction
#include <iAToolRegistry.h>

#include <QAction>
#include <QMenu>

void iALabellingModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	auto submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Labelling"), false);
	addToolAction<iALabellingTool>(m_mainWnd, submenu);
	addToolAction<iAAnnotationTool>(m_mainWnd, submenu);
	iAToolRegistry::addTool(iAAnnotationTool::Name, createTool<iAAnnotationTool>);
	iAToolRegistry::addTool(iALabellingTool::Name, createTool<iALabellingTool>);
}
