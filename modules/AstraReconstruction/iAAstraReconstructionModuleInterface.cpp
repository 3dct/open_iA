// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAstraReconstructionModuleInterface.h"

#include "iAAstraAlgorithm.h"

#include "iAFilterRunnerRegistry.h"

void iAAstraReconstructionModuleInterface::Initialize( )
{
	if (!m_mainWnd)
	{
		return;
	}
	iAFilterRunnerRegistry::add(iAFilterRegistry::filterID(iAASTRAForwardProject().name()), iAASTRAFilterRunner::create);
	iAFilterRunnerRegistry::add(iAFilterRegistry::filterID(iAASTRAReconstruct().name()), iAASTRAFilterRunner::create);
}
