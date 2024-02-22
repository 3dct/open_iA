// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGUIModuleInterface.h>

class iANModalTFModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;

private slots:
	void nModalTF();
	void modalitySPLOM();
};
