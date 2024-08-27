// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_UncertaintyToolBar.h"

#include <iAGUIModuleInterface.h>
#include <iAQTtoUIConnector.h>

#include <QToolBar>

class iAMainWindow;

typedef iAQTtoUIConnector<QToolBar, Ui_UncertaintyToolBar> iAUncertaintyToolbar;


class iAUncertaintyModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;

private:
	void setupToolBar();
	iAUncertaintyToolbar * m_toolbar = nullptr;
};
