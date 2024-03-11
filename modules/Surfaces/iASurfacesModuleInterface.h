// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGUIModuleInterface.h>

class iASurfacesModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize();
private slots:
	void addObject();
};
