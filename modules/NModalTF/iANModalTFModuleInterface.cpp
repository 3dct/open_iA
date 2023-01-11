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