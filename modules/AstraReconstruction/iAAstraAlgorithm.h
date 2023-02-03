// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAFilterDefault.h>
#include "iAFilterRunnerGUI.h"

IAFILTER_DEFAULT_CLASS(iAASTRAForwardProject);
IAFILTER_DEFAULT_CLASS(iAASTRAReconstruct);

enum AstraReconstructionMethods
{
	BP3D,
	FDK3D,
	SIRT3D,
	CGLS3D
};


class iAASTRAFilterRunner : public iAFilterRunnerGUI
{
public:
	static std::shared_ptr<iAFilterRunnerGUI> create();
	void run(std::shared_ptr<iAFilter> filter, iAMainWindow* mainWnd) override;
	bool askForParameters(std::shared_ptr<iAFilter> filter, QVariantMap & paramValues,
		iAMdiChild* sourceMdi, iAMainWindow* mainWnd, bool askForAdditionalInput) override;
};
