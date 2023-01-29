// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALabellingModuleInterface.h"

#include "iAAnnotationTool.h"
#include "iALabellingTool.h"

#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h>
#include <iAToolRegistry.h>

#include <QAction>
#include <QMenu>

void iALabellingModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	auto actionLabelling = new QAction(tr("Labelling"), m_mainWnd);
	connect(actionLabelling, &QAction::triggered, this, [this]()
		{
			addToolToActiveMdiChild<iALabellingTool>(iALabellingTool::Name, m_mainWnd);
		});

	auto actionAnnotation = new QAction(tr("Annotations"), m_mainWnd);
	connect(actionAnnotation, &QAction::triggered, this, [this]()
		{
			addToolToActiveMdiChild<iAAnnotationTool>(iAAnnotationTool::Name, m_mainWnd);
		});

	auto menuEnsembles = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Labelling"), false);
	menuEnsembles->addAction(actionLabelling);
	menuEnsembles->addAction(actionAnnotation);

	//iAToolRegistry::addTool(iAAnnotationTool::Name, iAAnnotationTool::create)
}
