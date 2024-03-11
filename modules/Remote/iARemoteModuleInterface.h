// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGUIModuleInterface.h>

class iARemoteModuleInterface : public iAGUIModuleInterface
{
public:
	void Initialize() override;

public slots:
	void addRemoteServer();
};
