// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iATripleHistogramTFModuleInterface.h"
#include "iATripleHistogramTFTool.h"

#include <iALog.h>

#include <iAToolHelper.h>    // for addToolToActiveMdiChild

#include <QAction>
#include <QMenu>

void iATripleHistogramTFModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}

	QAction *action_2mod = new QAction(tr("Double Histogram Transfer Function"), m_mainWnd);
	connect(action_2mod, &QAction::triggered, this, &iATripleHistogramTFModuleInterface::menuItemSelected_2mod);
	m_mainWnd->makeActionChildDependent(action_2mod);

	QAction *action_3mod = new QAction(tr("Triple Histogram Transfer Function"), m_mainWnd);
	connect(action_3mod, &QAction::triggered, this, &iATripleHistogramTFModuleInterface::menuItemSelected_3mod);
	m_mainWnd->makeActionChildDependent(action_3mod);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Multi-Modal/-Channel Images"), true);
	submenu->addAction(action_2mod);
	submenu->addAction(action_3mod);
}

iATripleHistogramTFTool* iATripleHistogramTFModuleInterface::getOrCreateTool()
{
	auto child = m_mainWnd->activeMdiChild();
	auto tool = getTool<iATripleHistogramTFTool>(child);
	if (!tool)
	{
		tool = addToolToActiveMdiChild<iATripleHistogramTFTool>("TripleHistogramTF", m_mainWnd);
		if (!tool)
		{
			LOG(lvlError, "Creating tool failed!");
		}
	}
	return tool;
}

void iATripleHistogramTFModuleInterface::menuItemSelected_2mod()
{
	auto tool = getOrCreateTool();
	if (tool)
	{
		tool->start2TF();
	}
}

void iATripleHistogramTFModuleInterface::menuItemSelected_3mod()
{
	auto tool = getOrCreateTool();
	if (tool)
	{
		tool->start3TF();
	}
}
