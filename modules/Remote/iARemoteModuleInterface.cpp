// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemoteModuleInterface.h"

// iAguibase
#include "iARenderer.h"
#include "iASlicerImpl.h"
#include "iASlicerMode.h"
#include "iATool.h"
#include "iAToolHelper.h"    // for addToolToActiveMdiChild

#include "iARemoteAction.h"
#include "iARemoteRenderer.h"
#include "iAWebsocketAPI.h"

#include <QAction>
#include <QDateTime>

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

namespace
{

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
}
#endif

class iARemoteTool: public QObject, public iATool
{
public:
	static const QString Name;
	iARemoteTool(iAMainWindow* mainWnd, iAMdiChild* child) :
		iATool(mainWnd, child),
		m_wsAPI(std::make_unique<iARemoteRenderer>(1234))
#ifdef QT_HTTPSERVER
		, m_httpServer(std::make_unique<QHttpServer>())
#endif
	{
		auto annotTool = getTool<iAAnnotationTool>(child);
		if (!annotTool)
		{
			auto newTool = std::make_shared<iAAnnotationTool>(mainWnd, child);
			child->addTool(iAAnnotationTool::Name, newTool);
			annotTool = newTool.get();
		}
		m_wsAPI->addRenderWindow(child->renderer()->renderWindow(), "3D");
		m_viewWidgets.insert("3D", child->rendererWidget());
		for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		{
			m_wsAPI->addRenderWindow(child->slicer(i)->renderWindow(), slicerModeString(i));
			child->slicer(i)->setShowTooltip(false);
			m_viewWidgets.insert(slicerModeString(i), child->slicer(i));
		}
		connect(m_wsAPI->m_websocket.get(), &iAWebsocketAPI::controlCommand, this, [this](iARemoteAction const & action) {
			//LOG(lvlDebug, QString("client control: action: %1; position: %2, %3")
			//	.arg((action.action == iARemoteAction::ButtonUp)?"up":"down")
			//	.arg(action.x)
			//	.arg(action.y)
			//	);
			int vtkEventID;
			if (action.action == iARemoteAction::MouseWheel || action.action == iARemoteAction::StartMouseWheel)
			{
				vtkEventID = (action.spinY < 0) ? vtkCommand::MouseWheelForwardEvent : vtkCommand::MouseWheelBackwardEvent;
			}
			else    // mouse button press or move
			{
				static bool lastDown = false;
				static qint64 lastInput = 0;
				vtkEventID = vtkCommand::MouseMoveEvent;
				bool curDown = (action.action == iARemoteAction::ButtonDown);
				auto now = QDateTime::currentMSecsSinceEpoch();
				auto millisecondsSinceLastInput = now - lastInput;
				if (lastDown != curDown || millisecondsSinceLastInput > 50)
				{
					lastInput = QDateTime::currentMSecsSinceEpoch();
				}
				else    // ignore mouse moves less than a few milliseconds apart from previous
				{
					return;
				}
				if (lastDown != curDown)
				{
					lastDown = curDown;
					vtkEventID = curDown ? vtkCommand::LeftButtonPressEvent : vtkCommand::LeftButtonReleaseEvent;
				}
			}
			auto renWin = m_wsAPI->renderWindow(action.viewID);
			if (!renWin)
			{
				return;
			}
			auto interactor = renWin->GetInteractor();
			interactor->SetControlKey(action.ctrlKey);
			interactor->SetShiftKey(action.shiftKey);
			interactor->SetAltKey(action.altKey);
			int const* size = renWin->GetSize();
			int pos[] = {static_cast<int>(size[0] * action.x), static_cast<int>(size[1] * action.y)};
			LOG(lvlDebug, QString("event id: %1; x: %2, y: %3").arg(vtkEventID).arg(pos[0]).arg(pos[1]));

			interactor->SetEventPosition(pos);
			//interactor->SetKeyCode(static_cast<char>(action.keyCode));
			//interactor->SetRepeatCount(repeatCount);
			//interactor->SetKeySym(keySym);

			interactor->InvokeEvent(vtkEventID, nullptr);

			//renWin->Render();
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
			LOG(lvlImportant, QString("You can reach the webserver under http://localhost:%1").arg(port));
		}
#endif

		connect(annotTool, &iAAnnotationTool::annotationsUpdated, m_wsAPI->m_websocket.get(),&iAWebsocketAPI::updateCaptionList);
		connect(m_wsAPI->m_websocket.get(), &iAWebsocketAPI::changeCaptionTitle, annotTool,
			&iAAnnotationTool::renameAnnotation);
		connect(m_wsAPI->m_websocket.get(), &iAWebsocketAPI::removeCaption, annotTool,
			&iAAnnotationTool::removeAnnotation);
		connect(m_wsAPI->m_websocket.get(), &iAWebsocketAPI::addMode, annotTool, &iAAnnotationTool::startAddMode);
		connect(
			m_wsAPI->m_websocket.get(), &iAWebsocketAPI::selectCaption, annotTool, &iAAnnotationTool::focusToAnnotation);
		connect(m_wsAPI->m_websocket.get(), &iAWebsocketAPI::hideAnnotation, annotTool,
			&iAAnnotationTool::hideAnnotation);
		connect(annotTool, &iAAnnotationTool::focusedToAnnotation, m_wsAPI->m_websocket.get(),
				&iAWebsocketAPI::sendInteractionUpdate);
	}

private:
	std::unique_ptr<iARemoteRenderer> m_wsAPI;
	QMap<QString, QWidget*> m_viewWidgets;
#ifdef QT_HTTPSERVER
	std::unique_ptr<QHttpServer> m_httpServer;
#endif
};

const QString iARemoteTool::Name("NDTflix");

void iARemoteModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction* actionRemote = new QAction(tr("Remote Render Server"), m_mainWnd);
	connect(actionRemote, &QAction::triggered, this,[this]()
		{
			addToolToActiveMdiChild<iARemoteTool>(iARemoteTool::Name, m_mainWnd);
		});
	m_mainWnd->makeActionChildDependent(actionRemote);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionRemote);
}
