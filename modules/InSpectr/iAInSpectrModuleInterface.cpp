// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAInSpectrModuleInterface.h"

#include "iAInSpectrTool.h"

#include <iAToolHelper.h>    // for addToolToActiveMdiChild

#include <QAction>

void iAInSpectrModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction * actionInSpectr = new QAction(tr("InSpectr"), m_mainWnd);
	connect(actionInSpectr, &QAction::triggered, this, &iAInSpectrModuleInterface::startInSpectr);
	m_mainWnd->makeActionChildDependent(actionInSpectr);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionInSpectr);
}

void iAInSpectrModuleInterface::startInSpectr()
{
	addToolToActiveMdiChild<iAInSpectrTool>(iAInSpectrTool::Name, m_mainWnd);
}
