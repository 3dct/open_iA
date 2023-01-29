// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iANModalTFModuleInterface.h"

#include "iANModalTool.h"
#include "iAModalitySPLOM.h"

#include <iADataSet.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <iALog.h>

#include <QAction>
#include <QMenu>

void iANModalTFModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction* actionNModalTF = new QAction(tr("n-Modal Transfer Function"), m_mainWnd);
	connect(actionNModalTF, &QAction::triggered, this, &iANModalTFModuleInterface::nModalTF);

	QAction * actionModalitySPLOM = new QAction(tr("Modality SPLOM"), m_mainWnd);
	connect(actionModalitySPLOM, &QAction::triggered, this, &iANModalTFModuleInterface::modalitySPLOM);
	m_mainWnd->makeActionChildDependent(actionModalitySPLOM);

	auto submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Multi-Modal/-Channel Images"), false);
	addToMenuSorted(submenu, actionNModalTF);
	addToMenuSorted(submenu, actionModalitySPLOM);
}

void iANModalTFModuleInterface::nModalTF()
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

void iANModalTFModuleInterface::modalitySPLOM()
{
	auto child = m_mainWnd->activeMdiChild();
	auto modalitySPLOM = new iAModalitySPLOM();
	std::vector<iAImageData*> dataSets;
	for (auto ds: child->dataSets())
	{
		auto imgDS = dynamic_cast<iAImageData*>(ds.get());
		if (imgDS) 
		{
			dataSets.push_back(imgDS);
		}
	}
	modalitySPLOM->setData(dataSets);
	child->tabifyDockWidget(child->renderDockWidget(), modalitySPLOM);
	// TODO NEWIO: add as tool?
}
