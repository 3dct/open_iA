// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGUIModuleInterface.h>
#include <iAAnnotationTool.h>

class iARemoteModuleInterface : public iAGUIModuleInterface
{
public:
	void Initialize() override;

public slots:
	void addRemoteServer();
};
