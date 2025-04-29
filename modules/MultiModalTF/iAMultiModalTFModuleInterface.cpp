// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMultiModalTFModuleInterface.h"

#include "iANModalTool.h"
#include "iAModalitySPLOM.h"
#include "iATripleHistogramTFTool.h"

#include <iAImageData.h>
#include <iAToolHelper.h>    // for addToolToActiveMdiChild

#include <iALog.h>

#include <QAction>
#include <QMenu>

namespace
{
	iATripleHistogramTFTool* tripleTFTool(iAMainWindow* mainWnd)
	{
		auto child = mainWnd->activeMdiChild();
		auto tool = getTool<iATripleHistogramTFTool>(child);
		if (!tool)
		{
			tool = addToolToActiveMdiChild<iATripleHistogramTFTool>("TripleHistogramTF", mainWnd);
			if (!tool)
			{
				LOG(lvlError, "Creating tool failed!");
			}
		}
		return tool;
	}
}

void iAMultiModalTFModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction* actionNModalTF = new QAction(tr("n-Modal Transfer Function"), m_mainWnd);
	actionNModalTF->setToolTip("Specify transfer functions through setting desired colors in slicers.");
	connect(actionNModalTF, &QAction::triggered, this, &iAMultiModalTFModuleInterface::nModalTF);

	QAction * actionModalitySPLOM = new QAction(tr("Modality SPLOM"), m_mainWnd);
	actionModalitySPLOM->setToolTip("Scatter Plot Matrix of multiple modalities.");
	connect(actionModalitySPLOM, &QAction::triggered, this, &iAMultiModalTFModuleInterface::modalitySPLOM);
	m_mainWnd->makeActionChildDependent(actionModalitySPLOM);

	QAction* action_2mod = new QAction(tr("Double Histogram Transfer Function"), m_mainWnd);
	action_2mod->setToolTip("Weighted fusion of two modalities.");
	connect(action_2mod, &QAction::triggered, this, &iAMultiModalTFModuleInterface::menuItemSelected_2mod);
	m_mainWnd->makeActionChildDependent(action_2mod);

	QAction *action_3mod = new QAction(tr("Triple Histogram Transfer Function"), m_mainWnd);
	action_3mod->setToolTip("Weighted fusion of three modalities based on joint histogram.");
	connect(action_3mod, &QAction::triggered, this, &iAMultiModalTFModuleInterface::menuItemSelected_3mod);
	m_mainWnd->makeActionChildDependent(action_3mod);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Multi-Modal/-Channel Images"), false);
	submenu->setToolTipsVisible(true);
	submenu->addAction(actionModalitySPLOM);
	submenu->addAction(actionNModalTF);
	submenu->addSeparator();
	submenu->addAction(action_2mod);
	submenu->addAction(action_3mod);
}

void iAMultiModalTFModuleInterface::nModalTF()
{
	auto tool = getTool<iANModalTFTool>(m_mainWnd->activeMdiChild());
	if (!tool)
	{
		tool = addToolToActiveMdiChild<iANModalTFTool>("NModalTF", m_mainWnd);
		if (!tool)
		{
			LOG(lvlError, "Creating tool failed!");
			return;
		}
	}
}

void iAMultiModalTFModuleInterface::modalitySPLOM()
{
	auto child = m_mainWnd->activeMdiChild();
	auto modalitySPLOM = new iAModalitySPLOM();
	std::vector<iAImageData*> dataSets;
	for (auto ds: child->dataSetMap())
	{
		auto imgDS = dynamic_cast<iAImageData*>(ds.second.get());
		if (imgDS)
		{
			dataSets.push_back(imgDS);
		}
	}
	modalitySPLOM->setData(dataSets);
	child->tabifyDockWidget(child->renderDockWidget(), modalitySPLOM);
	// TODO NEWIO: add as tool?
}

void iAMultiModalTFModuleInterface::menuItemSelected_2mod()
{
	auto tool = tripleTFTool(m_mainWnd);
	if (tool)
	{
		tool->start2TF();
	}
}

void iAMultiModalTFModuleInterface::menuItemSelected_3mod()
{
	auto tool = tripleTFTool(m_mainWnd);
	if (tool)
	{
		tool->start3TF();
	}
}
