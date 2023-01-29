// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_UncertaintyToolBar.h"

#include <iAGUIModuleInterface.h>
#include <qthelper/iAQTtoUIConnector.h>

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
