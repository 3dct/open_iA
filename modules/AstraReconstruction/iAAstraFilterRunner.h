// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFilterRunnerGUI.h"

class iAAstraFilterRunner : public iAFilterRunnerGUI
{
public:
	static std::shared_ptr<iAFilterRunnerGUI> create();
	void run(std::shared_ptr<iAFilter> filter, iAMainWindow* mainWnd) override;
	bool askForParameters(std::shared_ptr<iAFilter> filter, QVariantMap & paramValues,
		iAMdiChild* sourceMdi, iAMainWindow* mainWnd, bool askForAdditionalInput) override;
};
