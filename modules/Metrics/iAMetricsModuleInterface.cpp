// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMetricsModuleInterface.h"

#include "iAQMeasure.h"

void iAMetricsModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	iAFilterRunnerRegistry::add(iAFilterRegistry::filterID(iAQMeasure().name()), iAQMeasureRunner::create);
}
