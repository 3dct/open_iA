// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_SPMSettings.h"

class iASPMSettings : public QDialog, public Ui_SPMSettings
{
	Q_OBJECT
public:
	iASPMSettings(QWidget * parent = nullptr) : QDialog(parent)
	{
		setupUi(this);
	}
};
