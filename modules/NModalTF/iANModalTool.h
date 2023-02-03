// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

class iADockWidgetWrapper;
class iAMdiChild;

class iANModalTFTool : public iATool
{
public:
	iANModalTFTool(iAMainWindow* mainWnd, iAMdiChild* child);

private:
	iADockWidgetWrapper* m_nModalDockWidget;
};
