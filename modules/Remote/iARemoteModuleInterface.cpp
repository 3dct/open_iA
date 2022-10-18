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
#include "iARenderer.h"
#include "iASlicer.h"

#include "iARemoteAction.h"
#include "iARemoteRenderer.h"
#include "iAWebsocketAPI.h"

#include <QAction>

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyle.h>
#include <vtkRenderWindowInteractor.h>

#ifdef QT_HTTPSERVER

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QString>
#include <QHttpServer>
#include <QTextStream>

void addFileToServe(QString path, QString query, QString fileName, QHttpServer* server, QString mimeType)
{
	auto routeCreated = server->route(query,
		[path, fileName, mimeType](QHttpServerResponder&& responder)
	{
		QFile fileToServe(path + "/" + fileName);
		if (!fileToServe.open(QFile::ReadOnly | QFile::Text))
		{
			LOG(lvlError, QString("Could not open file to server (%1) in given path (%2).").arg(fileName).arg(path));
		}
		QTextStream in(&fileToServe);
		auto value = in.readAll();
		responder.write(value.toUtf8(), mimeType.toUtf8());
	});

	if (!routeCreated)
	{
		LOG(lvlError, QString("Error creating server route for file %1").arg(fileName));
	}
}

void addDirectorytoServer(QString path, QHttpServer* server)
{

	QDir directory(path);
	QStringList files = directory.entryList(QDir::Files);
	for (QString filename : files)
	{
		if (filename.contains("index.html"))
		{
			addFileToServe(path, "/", filename, server, "text/html");
			addFileToServe(path, "/" + filename, filename, server, "text/html");
		}
		else if (filename.endsWith("css", Qt::CaseInsensitive))
		{
			addFileToServe(path, "/" + filename, filename, server, "text/css");
		}
		else if (filename.endsWith("html", Qt::CaseInsensitive))
		{
			addFileToServe(path, "/" + filename, filename, server, "text/html");
		}
		else if (filename.endsWith("js", Qt::CaseInsensitive))
		{
			addFileToServe(path, "/" + filename, filename, server, "application/javascript");
		}
		else
		{
			addFileToServe(path, "/" + filename, filename, server, "text/plain");
		}
	}
}


#endif

class iARemoteAttachment: public iAModuleAttachmentToChild
{
public:
	iARemoteAttachment(iAMainWindow* mainWnd, iAMdiChild* child):
		iAModuleAttachmentToChild(mainWnd, child),
		m_wsAPI(std::make_unique<iARemoteRenderer>(1234))
#ifdef QT_HTTPSERVER
		, m_httpServer(std::make_unique<QHttpServer>())
#endif
	{


		m_wsAPI->addRenderWindow(child->renderer()->renderWindow(), "3D");
		m_wsAPI->addRenderWindow(child->slicer(iASlicerMode::XY)->renderWindow(), "XY");
		m_wsAPI->addRenderWindow(child->slicer(iASlicerMode::XZ)->renderWindow(), "XZ");
		m_wsAPI->addRenderWindow(child->slicer(iASlicerMode::YZ)->renderWindow(), "YZ");
		m_viewWidgets.insert("3D", child->rendererWidget());
		m_viewWidgets.insert("XY", child->slicer(iASlicerMode::XY));
		m_viewWidgets.insert("XZ", child->slicer(iASlicerMode::XZ));
		m_viewWidgets.insert("YZ", child->slicer(iASlicerMode::YZ));

		connect(m_wsAPI->m_websocket.get(), &iAWebsocketAPI::controlCommand, this, [this](iARemoteAction const & action) {
			static bool lastDown = false;
			static qint64 lastInput = 0;
			//LOG(lvlDebug, QString("client control: action: %1; position: %2, %3")
			//	.arg((action.action == iARemoteAction::up)?"up":"down")
			//	.arg(action.x)
			//	.arg(action.y)
			//	);
			auto now = QDateTime::currentMSecsSinceEpoch();
			bool curDown = (action.action == iARemoteAction::down);

			auto millisecondsSinceLastInput = now - lastInput;

			if (lastDown != curDown || millisecondsSinceLastInput > 50)
			{
				lastInput = QDateTime::currentMSecsSinceEpoch();
			}
			else
			{
				return;
			}
			int eventID = vtkCommand::MouseMoveEvent;
			if (lastDown != curDown)
			{
				lastDown = curDown;
				eventID = curDown ? vtkCommand::LeftButtonPressEvent : vtkCommand::LeftButtonReleaseEvent;
			}
			auto renWin = m_wsAPI->renderWindow(action.viewID);
			auto interactor = renWin->GetInteractor();
			interactor->SetControlKey(action.ctrlKey);
			interactor->SetShiftKey(action.shiftKey);
			interactor->SetAltKey(action.altKey);
			int const* size = renWin->GetSize();
			int pos[] = {static_cast<int>(size[0] * action.x), static_cast<int>(size[1] * action.y)};
			
			if (action.action == iARemoteAction::MouseWheel)
			{
				if (action.spinY > 0)
				{
					interactor->MouseWheelForwardEvent();
				}
				else
				{
					interactor->MouseWheelBackwardEvent();
				}


			}
			
			//LOG(lvlDebug, QString("event id: %1; x: %2, y: %3").arg(eventID).arg(pos[0]).arg(pos[1]));
			interactor->SetEventPosition(pos);
			//interactor->SetKeyCode(static_cast<char>(action.keyCode));
			//interactor->SetRepeatCount(repeatCount);
			//interactor->SetKeySym(keySym);

			interactor->InvokeEvent(eventID, nullptr);

			renWin->Render();
			m_viewWidgets[action.viewID]->update();

			//interactor()->Modified();
			//interactor()->Render();
			
		});

#ifdef QT_HTTPSERVER

		QString path = QCoreApplication::applicationDirPath() + "/RemoteClient";

		addDirectorytoServer(path, m_httpServer.get());

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
	std::unique_ptr<iARemoteRenderer> m_wsAPI;
	QMap<QString, QWidget*> m_viewWidgets;
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
