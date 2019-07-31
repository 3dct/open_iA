/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iANModalMain.h"

#include <iAConsole.h>
#include <mainwindow.h>
#include <mdichild.h>

void iANModalTFModuleInterface::Initialize() {
	if (!m_mainWnd) // if m_mainWnd is not set, we are running in command line mode
		return;     // in that case, we do not do anything as we can not add a menu entry there
	QMenu *toolsMenu = m_mainWnd->toolsMenu();
	QMenu *menuMultiModalChannel = getMenuWithTitle(toolsMenu, QString("Multi-Modal/-Channel Images"), false);

	QAction *action = new QAction(m_mainWnd);
	action->setText(QApplication::translate("MainWindow", "n-Modal Transfer Function", nullptr));
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, action, true);
	connect(action, SIGNAL(triggered()), this, SLOT(onMenuItemSelected()));
}

iAModuleAttachmentToChild* iANModalTFModuleInterface::CreateAttachment(MainWindow* mainWnd, MdiChild *childData) {
	return iANModalAttachment::create(mainWnd, childData);
}

void iANModalTFModuleInterface::onMenuItemSelected() {
	PrepareActiveChild();
	auto attach = attachment<iANModalAttachment>(m_mdiChild);
	if (!attach)
	{
		AttachToMdiChild(m_mdiChild);
		attach = attachment<iANModalAttachment>(m_mdiChild);
		if (!attach)
		{
			DEBUG_LOG("Attaching failed!");
			return;
		}
	}
	attach->start();
}

