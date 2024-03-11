// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_GEMSeToolBar.h"

#include <iAGUIModuleInterface.h>
#include <qthelper/iAQTtoUIConnector.h>

#include <QToolBar>

typedef iAQTtoUIConnector<QToolBar, Ui_GEMSeToolBar> iAGEMSeToolbar;

class iAGEMSeModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	iAGEMSeModuleInterface();
	void Initialize() override;
	void setupToolbar();
private:
	iAGEMSeToolbar* m_toolbar;
};
