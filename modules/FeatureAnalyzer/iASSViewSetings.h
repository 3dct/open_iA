// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_SSSettings.h"

#include <qthelper/iAQTtoUIConnector.h>

typedef iAQTtoUIConnector<QDialog, Ui_SSSettings>  SSSettingsConnector;

class iASSViewSettings : public SSSettingsConnector
{
	Q_OBJECT
public:
	iASSViewSettings(QWidget* parent = nullptr) : SSSettingsConnector(parent)
	{}
	~iASSViewSettings() {}
};
