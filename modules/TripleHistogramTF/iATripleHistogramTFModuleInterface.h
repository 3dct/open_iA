// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGUIModuleInterface.h>

class dlg_tf_2mod;
class dlg_tf_3mod;

class iATripleHistogramTFTool;

class iATripleHistogramTFModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private:
	iATripleHistogramTFTool* getOrCreateTool();
private slots:
	void menuItemSelected_2mod();
	void menuItemSelected_3mod();
};
