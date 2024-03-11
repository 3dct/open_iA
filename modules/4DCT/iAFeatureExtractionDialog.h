// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_iAFeatureExtraction.h"
// proj
#include "iACommonInput.h"

class iAFeatureExtractionDialog : public iACommonInput
{
	Q_OBJECT
public:
				iAFeatureExtractionDialog(QWidget* parent = 0);
				~iAFeatureExtractionDialog();

	QString		getInputImg();
	QString		getOutputFile();

private:
	Ui::iAFeatureExtractionInput	ui;
};
