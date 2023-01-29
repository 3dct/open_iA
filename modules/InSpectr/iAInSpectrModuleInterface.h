// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGUIModuleInterface.h>

class iAInSpectrModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private:
	void startInSpectr();
};
