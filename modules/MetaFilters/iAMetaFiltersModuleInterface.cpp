// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMetaFiltersModuleInterface.h"

#include "iAFilterRegistry.h"
#include "iAFilterRunnerRegistry.h"

#include "iASampleFilter.h"

void iAMetaFiltersModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	iAFilterRunnerRegistry::add(iAFilterRegistry::filterID(iASampleFilter().name()), iASampleFilterRunnerGUI::create);
}
