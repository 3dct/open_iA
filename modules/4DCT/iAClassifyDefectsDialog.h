// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// qt
// ui
#include "ui_iAClassifyDefectsDialog.h"

class iAClassifyDefectsDialog : public QDialog
{
	Q_OBJECT

public:
	iAClassifyDefectsDialog();
	~iAClassifyDefectsDialog();
	Ui::ClassifyDefects ui;
};
