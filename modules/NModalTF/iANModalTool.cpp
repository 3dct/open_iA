// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iANModalTool.h"

#include "iANModalWidget.h"

#include <iADockWidgetWrapper.h>
#include <iALog.h>
#include <iAMdiChild.h>

iANModalTFTool::iANModalTFTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_nModalDockWidget(new iADockWidgetWrapper(new iANModalWidget(child), "n-Modal Transfer Function", "nModalTF"))
{
	m_child->tabifyDockWidget(m_child->renderDockWidget(), m_nModalDockWidget);
}
