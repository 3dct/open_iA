// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFilterRunnerRegistry.h"

#include "iAFilterRunnerGUI.h"

#include <QMap>

namespace
{
	QMap<int, iAFilterRunnerGUICreateFuncPtr> & runners()
	{
		static QMap<int, iAFilterRunnerGUICreateFuncPtr> s_runner;
		return s_runner;
	}
}

void iAFilterRunnerRegistry::add(int filterID, iAFilterRunnerGUICreateFuncPtr runnerCreateFunc)
{
	runners().insert(filterID, runnerCreateFunc);
}

std::shared_ptr<iAFilterRunnerGUI> iAFilterRunnerRegistry::filterRunner(int filterID)
{
	if (runners().contains(filterID))
	{
		return runners()[filterID]();
	}
	else
	{
		return std::make_shared<iAFilterRunnerGUI>();
	}
}
