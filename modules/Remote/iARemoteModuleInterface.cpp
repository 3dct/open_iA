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

#ifdef QT_HTTPSERVER

#include <QCoreApplication>
#include <QFile>
#include <QHttpServer>
#include <QTextStream>

void addFileToServe(QString path, QString query, QString fileName, QHttpServer* server)
{
	auto routeCreated = server->route(query, [path, fileName]() -> QString  {
		QFile fileToServe(path + "/" + fileName);
		if (!fileToServe.open(QFile::ReadOnly | QFile::Text))
		{
			LOG(lvlError, QString("Could not open file to server (%1) in given path (%2).").arg(fileName).arg(path));
			return "Error";
		}
		QTextStream in(&fileToServe);
		auto value = in.readAll();
		return value;
	});
	if (!routeCreated)
	{
		LOG(lvlError, QString("Error creating server route for file %1").arg(fileName));
	}
}

#endif

class iARemoteAttachment: public iAModuleAttachmentToChild
{
public:
	iARemoteAttachment(iAMainWindow* mainWnd, iAMdiChild* child):
		iAModuleAttachmentToChild(mainWnd, child),
		m_wsAPI(std::make_unique<iAWebsocketAPI>(1234))
#ifdef QT_HTTPSERVER
		, m_httpServer(std::make_unique<QHttpServer>())
#endif
	{
#ifdef QT_HTTPSERVER
		QString path = QCoreApplication::applicationDirPath() + "/RemoteClient";
		addFileToServe(path, "/", "index.html", m_httpServer.get());
		addFileToServe(path, "/main.js", "main.js", m_httpServer.get());
		auto port = m_httpServer->listen(QHostAddress::Any, 8080);
		if (port == 0)
		{
			LOG(lvlError, "Could not bind server!");
		}
		auto ports = m_httpServer->serverPorts();
		if (ports.size() != 1)
		{
			LOG(lvlError, "Invalid number of server ports!");
		}
		if (port == 8080)
		{
			LOG(lvlImportant, QString("You can reach the webserver under http:\\\\localhost:%1").arg(port));
		}

#endif
	}
private:
	std::unique_ptr<iAWebsocketAPI> m_wsAPI;
#ifdef QT_HTTPSERVER
	std::unique_ptr<QHttpServer> m_httpServer;
#endif
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
