
// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAFilterRunnerGUI.h>

#include <QVariantMap>

#include <memory>

class iAFilter;
class iAMainWindow;
class iAMdiChild;

class iASampleFilterRunnerGUI : public iAFilterRunnerGUI
{
public:
	static std::shared_ptr<iAFilterRunnerGUI> create();
	bool askForParameters(std::shared_ptr<iAFilter> filter, QVariantMap& paramValues,
		iAMdiChild* sourceMdi, iAMainWindow* mainWnd, bool askForAdditionalInput) override;
};
