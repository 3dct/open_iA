/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAModalityExplorerModuleInterface.h"

#include "iAModalityExplorerAttachment.h"

#include <iALog.h>
#include <mainwindow.h>
#include <mdichild.h>

void iAModalityExplorerModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction * actionModalitySPLOM = new QAction(tr("Modality SPLOM"), m_mainWnd);
	connect(actionModalitySPLOM, &QAction::triggered, this, &iAModalityExplorerModuleInterface::ModalitySPLOM);
	makeActionChildDependent(actionModalitySPLOM);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Multi-Modal/-Channel Images"));
	addToMenuSorted(submenu, actionModalitySPLOM);
}


iAModuleAttachmentToChild* iAModalityExplorerModuleInterface::CreateAttachment(MainWindow* mainWnd, MdiChild * child)
{
	iAModalityExplorerAttachment* result = iAModalityExplorerAttachment::create( mainWnd, child);
	return result;
}


void iAModalityExplorerModuleInterface::ModalitySPLOM()
{
	PrepareActiveChild();
	bool result = AttachToMdiChild(m_mdiChild);
	iAModalityExplorerAttachment* attach = GetAttachment<iAModalityExplorerAttachment>();
	if (!result || !attach)
	{
		LOG(lvlInfo, "ModalityExplorer could not be initialized!");
	}
}
