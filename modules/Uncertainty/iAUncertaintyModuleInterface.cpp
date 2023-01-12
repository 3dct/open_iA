/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAUncertaintyModuleInterface.h"

#include "iAUncertaintyTool.h"

#include <iALog.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAToolRegistry.h>

#include <QMenu>

void iAUncertaintyModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	iAToolRegistry::addTool("Uncertainty", &iAUncertaintyTool::create);

	QAction * actionUncertainty = new QAction(tr("Uncertainty Exploration"), m_mainWnd);
	connect(actionUncertainty, &QAction::triggered, this,
	[this]()
	{
		setupToolBar();
		auto child = m_mainWnd->createMdiChild(false);
		auto tool = std::make_shared<iAUncertaintyTool>(m_mainWnd, child);
		child->addTool("Uncertainty", tool);
		// TODO: We need to create a new iAEnsemble / iAEnsembleDescriptorFile from a user-selected sampling (.smp) file!
		child->show();
	});

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Image Ensembles"), true);
	submenu->addAction(actionUncertainty);
}

void iAUncertaintyModuleInterface::setupToolBar()
{
	if (m_toolbar)
	{
		return;
	}
	m_toolbar = new iAUncertaintyToolbar("Uncertainty Exploration Toolbar");
	m_toolbar->action_ToggleSettings->setCheckable(true);
	m_toolbar->action_ToggleSettings->setChecked(true);
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolbar);

	auto toolAction = [this](auto toolBarAction)
	{
		auto tool = getTool<iAUncertaintyTool>(m_mainWnd->activeMdiChild());
		if (!tool)
		{
			LOG(lvlError, "Uncertainty exploration was not loaded properly!");
			return;
		}
		std::invoke(toolBarAction, tool);
	};
	connect(m_toolbar->action_ToggleTitleBar, &QAction::triggered, this, [toolAction]() {
		toolAction(&iAUncertaintyTool::toggleDockWidgetTitleBars);
	});
	connect(m_toolbar->action_ToggleSettings, &QAction::triggered, this, [toolAction]() {
		toolAction(&iAUncertaintyTool::toggleSettings);
	});
	connect(m_toolbar->action_CalculateNewSubEnsemble, &QAction::triggered, this, [toolAction]() {
		toolAction(&iAUncertaintyTool::calculateNewSubEnsemble);
	});
	connect(m_toolbar->action_WriteFullDataFile, &QAction::triggered, this, [toolAction]() {
		toolAction(&iAUncertaintyTool::writeFullDataFile);
		});
}
