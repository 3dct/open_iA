// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemoteModuleInterface.h"

// iAguibase
#include "iARenderer.h"
#include "iASlicer.h"
#include "iASlicerMode.h"
#include "iATool.h"
#include "iAToolHelper.h"    // for addToolToActiveMdiChild

#include "iARemoteAction.h"
#include "iARemoteRenderer.h"
#include "iAWebsocketAPI.h"

#include <QAction>
#include <QDateTime>

#include <vtkRenderWindow.h>


#ifdef QT_HTTPSERVER

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QString>
#include <QHttpServer>
#include <QTextStream>

namespace
{

	void addFileToServe(QString path, QString query, QString fileName, QHttpServer* server)
	{
		auto routeCreated = server->route(query, [path, fileName](QHttpServerRequest const& request)
			{
				Q_UNUSED(request);
				auto fileToServe = path + "/" + fileName;
				if (!QFile::exists(fileToServe))
				{
					LOG(lvlError, QString("Could not open file to serve (%1) in given path (%2).").arg(fileName).arg(path));
				}
				return QHttpServerResponse::fromFile(path + "/" + fileName);
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
		addFileToServe(path, "/", "index.html", server);
		for (QString filename : files)
		{
			addFileToServe(path, "/" + filename, filename, server);
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
		m_remoteRenderer(std::make_unique<iARemoteRenderer>(1234))
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
		m_remoteRenderer->addRenderWindow(child->renderer()->renderWindow(), "3D");
		m_viewWidgets.insert("3D", child->rendererWidget());
		for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		{
			m_remoteRenderer->addRenderWindow(child->slicer(i)->renderWindow(), slicerModeString(i));
			child->slicer(i)->setShowTooltip(false);
			m_viewWidgets.insert(slicerModeString(i), child->slicer(i));
		}
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::controlCommand, this, [this]()
		{
			//QSet<QString> viewIDs;
			auto actions = m_remoteRenderer->m_wsAPI->getQueuedActions();
			LOG(lvlDebug, QString("ControlCommand: %1 actions to process.").arg(actions.size()));
			for (auto action: actions)
			{
				//viewIDs.insert(action->viewID);
				int vtkEventID;
				if (action->action == iARemoteAction::MouseWheel || action->action == iARemoteAction::StartMouseWheel)
				{
					vtkEventID = (action->spinY < 0) ? vtkCommand::MouseWheelForwardEvent : vtkCommand::MouseWheelBackwardEvent;
				}
				else    // mouse button press or move
				{
					static bool lastDown = false;
					static qint64 lastInput = 0;
					vtkEventID = vtkCommand::MouseMoveEvent;
					bool curDown = (action->action == iARemoteAction::ButtonDown);
					auto now = QDateTime::currentMSecsSinceEpoch();
					auto millisecondsSinceLastInput = now - lastInput;
					if (lastDown != curDown || millisecondsSinceLastInput > 50)
					{
						lastInput = QDateTime::currentMSecsSinceEpoch();
					}
					else    // ignore mouse moves less than a few milliseconds apart from previous
					{
						LOG(lvlDebug, "Input ignored!");
						return;
					}
					if (lastDown != curDown)
					{
						lastDown = curDown;
						vtkEventID = curDown ? vtkCommand::LeftButtonPressEvent : vtkCommand::LeftButtonReleaseEvent;
					}
				}
				auto renWin = m_remoteRenderer->renderWindow(action->viewID);
				if (!renWin)
				{
					LOG(lvlError, QString("No render window for view %1").arg(action->viewID));
					return;
				}
				auto interactor = renWin->GetInteractor();
				interactor->SetControlKey(action->ctrlKey);
				interactor->SetShiftKey(action->shiftKey);
				interactor->SetAltKey(action->altKey);
				int const* size = renWin->GetSize();
				int pos[] = {static_cast<int>(size[0] * action->x), static_cast<int>(size[1] * action->y)};
				//LOG(lvlDebug, QString("event id: %1; x: %2, y: %3").arg(vtkEventID).arg(pos[0]).arg(pos[1]));

				interactor->SetEventPosition(pos);
				//interactor->SetKeyCode(static_cast<char>(action.keyCode));
				//interactor->SetRepeatCount(repeatCount);
				//interactor->SetKeySym(keySym);

				interactor->InvokeEvent(vtkEventID, nullptr);
				delete action;    // clean up action
			}

			//for (auto viewID : viewIDs)
			//{
			//	m_viewWidgets[viewID]->update();
			//}
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

		connect(annotTool, &iAAnnotationTool::annotationsUpdated, m_remoteRenderer->m_wsAPI.get(),&iAWebsocketAPI::updateCaptionList);
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::changeCaptionTitle, annotTool,	&iAAnnotationTool::renameAnnotation);
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::removeCaption, annotTool, &iAAnnotationTool::removeAnnotation);
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::addMode, annotTool, &iAAnnotationTool::startAddMode);
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::selectCaption, annotTool, &iAAnnotationTool::focusToAnnotation);
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::hideAnnotation, annotTool,	&iAAnnotationTool::hideAnnotation);
		connect(annotTool, &iAAnnotationTool::focusedToAnnotation, m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::sendInteractionUpdate);
	}

private:
	std::unique_ptr<iARemoteRenderer> m_remoteRenderer;
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
			// cannot start remote server twice - hard-coded websocket ports!
			for (auto c : m_mainWnd->mdiChildList())
			{
				if (getTool<iARemoteTool>(c))
				{
					LOG(lvlWarn, "Remote render server already running!");
					return;
				}
			}
			// TODO: - find way to inject websocket port into served html? we could modify served file on the fly...
			//       - then we would need to modify above check to only check for current child (no sense in serving same child twice)
			addToolToActiveMdiChild<iARemoteTool>(iARemoteTool::Name, m_mainWnd);
		});
	m_mainWnd->makeActionChildDependent(actionRemote);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionRemote);
}
