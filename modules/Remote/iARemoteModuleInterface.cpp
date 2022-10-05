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
#include "iARemoteModuleInterface.h"

// iAguibase
#include "iAMainWindow.h"
#include "iAMdiChild.h"
#include "iAModuleAttachmentToChild.h"

#include "iAWebsocketAPI.h"

class iARemoteAttachment: public iAModuleAttachmentToChild
{
public:
	iARemoteAttachment(iAMainWindow* mainWnd, iAMdiChild* child):
		iAModuleAttachmentToChild(mainWnd, child),
		m_wsAPI(std::make_unique<iAWebsocketAPI>(1234))
	{
	}
private:
	std::unique_ptr<iAWebsocketAPI> m_wsAPI;
};

void iARemoteModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction* actionRemote = new QAction(tr("Remote Render Server"), m_mainWnd);
	connect(actionRemote, &QAction::triggered, this, &iARemoteModuleInterface::addRemoteServer);
	m_mainWnd->makeActionChildDependent(actionRemote);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionRemote);
}

iAModuleAttachmentToChild* iARemoteModuleInterface::CreateAttachment(iAMainWindow* mainWnd, iAMdiChild* child)
{
	return new iARemoteAttachment(mainWnd, child);
}

void iARemoteModuleInterface::addRemoteServer()
{
	if (!m_mainWnd->activeMdiChild())
	{
		return;
	}
	AttachToMdiChild(m_mainWnd->activeMdiChild());
}
