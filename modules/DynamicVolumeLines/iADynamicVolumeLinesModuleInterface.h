// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGUIModuleInterface.h>

class dlg_DynamicVolumeLines;

class iADynamicVolumeLinesModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT

public:
	void Initialize() override;

private slots:
	void DynamicVolumeLines();

private:
	dlg_DynamicVolumeLines* dc;
};
