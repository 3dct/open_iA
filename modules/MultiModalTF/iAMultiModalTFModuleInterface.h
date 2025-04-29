// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGUIModuleInterface.h>

class iAMultiModalTFModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private slots:
	void nModalTF();
	void modalitySPLOM();
	void menuItemSelected_2mod();
	void menuItemSelected_3mod();
};
