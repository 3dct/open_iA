// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iATool.h"

class iAVolumePlayerWidget;

class iAVolumePlayerTool : public iATool
{
public:
	static const QString Name;
	iAVolumePlayerTool(iAMainWindow* wnd, iAMdiChild* child);
private:
	iAVolumePlayerWidget* m_volumePlayer;
};
