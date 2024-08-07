// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAGUIModuleInterface.h"

class iABoneThicknessModuleInterface : public iAGUIModuleInterface
{
public:
	void Initialize() override;
private slots:
	void slotBoneThickness();
};
