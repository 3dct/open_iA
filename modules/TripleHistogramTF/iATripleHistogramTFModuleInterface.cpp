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

#include "iATripleHistogramTFModuleInterface.h"
#include "iATripleHistogramTFAttachment.h"

#include <iALog.h>
#include <mainwindow.h>

void iATripleHistogramTFModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}

	QAction *action_2mod = new QAction(tr("Double Histogram Transfer Function"), m_mainWnd);
	connect(action_2mod, &QAction::triggered, this, &iATripleHistogramTFModuleInterface::menuItemSelected_2mod);
	makeActionChildDependent(action_2mod);

	QAction *action_3mod = new QAction(tr("Triple Histogram Transfer Function"), m_mainWnd);
	connect(action_3mod, &QAction::triggered, this, &iATripleHistogramTFModuleInterface::menuItemSelected_3mod);
	makeActionChildDependent(action_3mod);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Multi-Modal/-Channel Images"), true);
	submenu->addAction(action_2mod);
	submenu->addAction(action_3mod);
}

iAModuleAttachmentToChild* iATripleHistogramTFModuleInterface::CreateAttachment(MainWindow* mainWnd, MdiChild* child)
{
	return iATripleHistogramTFAttachment::create(mainWnd, child);
}

void iATripleHistogramTFModuleInterface::menuItemSelected_2mod()
{
	PrepareActiveChild();
	auto attach = GetAttachment<iATripleHistogramTFAttachment>();
	if (!attach)
	{
		AttachToMdiChild(m_mdiChild);
		attach = GetAttachment<iATripleHistogramTFAttachment>();
		if (!attach)
		{
			LOG(lvlInfo, "Attaching failed!");
			return;
		}
	}
	attach->start2TF();
}

void iATripleHistogramTFModuleInterface::menuItemSelected_3mod()
{
	PrepareActiveChild();
	auto attach = GetAttachment<iATripleHistogramTFAttachment>();
	if (!attach)
	{
		AttachToMdiChild(m_mdiChild);
		attach = GetAttachment<iATripleHistogramTFAttachment>();
		if (!attach)
		{
			LOG(lvlInfo, "Attaching failed!");
			return;
		}
	}
	attach->start3TF();
}
