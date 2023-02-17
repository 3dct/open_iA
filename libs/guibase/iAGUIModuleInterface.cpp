// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGUIModuleInterface.h"

#include "iALog.h"
#include "iAMainWindow.h"
#include "iAMdiChild.h"

#include <QStatusBar>
#include <QString>

iAGUIModuleInterface::~iAGUIModuleInterface()
{}

void iAGUIModuleInterface::SetMainWindow( iAMainWindow * mainWnd )
{
	m_mainWnd = mainWnd;
}

iAGUIModuleInterface::iAGUIModuleInterface():
	m_mainWnd(nullptr)
{}

void iAGUIModuleInterface::SaveSettings() const {}
