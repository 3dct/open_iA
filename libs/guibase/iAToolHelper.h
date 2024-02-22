// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iALog.h>

#include "iAMainWindow.h"
#include "iAMdiChild.h"

//! helper function to add a tool to the current mdi child (if it exists)
template <typename T>
T* addToolToActiveMdiChild(QString const & name, iAMainWindow* mainWnd)
{
	auto child = mainWnd->activeMdiChild();
	if (!child)
	{
		LOG(lvlWarn, "addTool: child not set!");
		return nullptr;
	}
	auto t = std::make_shared<T>(mainWnd, child);
	child->addTool(name, t);
	return t.get();
}
