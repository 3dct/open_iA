// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemoteModuleInterface.h"

#include "iARemoteAction.h"
#include "iARemoteRenderer.h"
#include "iAWebsocketAPI.h"

// iAguibase
#include "iADockWidgetWrapper.h"
#include "iARenderer.h"
#include "iASlicer.h"
#include "iASlicerMode.h"
#include "iATool.h"
#include "iAToolHelper.h"    // for addToolToActiveMdiChild

// base
#include "iAStringHelper.h"
#include "iAWebSocketServerTool.h"

#include <QAction>
#include <QDateTime>
#include <QHeaderView>
#include <QTableWidget>

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

namespace
{
	QString mouseActionToString(iARemoteAction::mouseAction action)
	{
		switch (action)
		{
		case iARemoteAction::ButtonDown:      return "Mouse button down";
		case iARemoteAction::ButtonUp:        return "Mouse button up";
		case iARemoteAction::StartMouseWheel: return "Mouse wheel start";
		case iARemoteAction::MouseWheel:      return "Mouse wheel continue";
		case iARemoteAction::EndMouseWheel:   return "Mouse wheel end";
		default:
		case iARemoteAction::Unknown:         return "Unknown";
		}
	}
}

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
			// TODO: potential for speedup: avoid dynamic allocation here!
			auto actions = m_remoteRenderer->m_wsAPI->getQueuedActions();
			static bool lastDown = false;
			for (int cur = 0; cur < actions.size(); ++cur)
			{
				auto action = actions[cur];
				LOG(lvlDebug, QString("Action: %1; position: %2, %3; shift: %4; ctrl: %5; alt: %6.")
					.arg(mouseActionToString(action->action))
					.arg(action->x)
					.arg(action->y)
					.arg(action->shiftKey)
					.arg(action->ctrlKey)
					.arg(action->altKey)
				);
				int vtkEventID;
				if (action->action == iARemoteAction::MouseWheel || action->action == iARemoteAction::StartMouseWheel)
				{
					vtkEventID = (action->spinY < 0) ? vtkCommand::MouseWheelForwardEvent : vtkCommand::MouseWheelBackwardEvent;
				}
				else    // mouse button press or move
				{
					vtkEventID = vtkCommand::MouseMoveEvent;
					bool curDown = (action->action == iARemoteAction::ButtonDown);
					bool nextDown = (cur < actions.size() - 1 &&
						actions[cur + 1]->action == iARemoteAction::ButtonDown);
					// TODO: check whether left/middle/right button matches?
					if (lastDown != curDown)
					{
						//LOG(lvlDebug, QString("Processing mouse up/down x=%1, y=%2").arg(action->x).arg(action->y));
						lastDown = curDown;
						vtkEventID = curDown ? vtkCommand::LeftButtonPressEvent : vtkCommand::LeftButtonReleaseEvent;
					}
					else if (curDown&& nextDown)
					{   // next action is also a mouse move, skip this one!
						//LOG(lvlDebug, QString("Skipping mouse move x=%1, y=%2").arg(action->x).arg(action->y));
						continue;
					}
					else
					{
						//LOG(lvlDebug, QString("Processing mouse move x=%1, y=%2").arg(action->x).arg(action->y));
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
				delete action;    // clean up action;
			}
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
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::changeCaptionTitle, annotTool, &iAAnnotationTool::renameAnnotation);
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::removeCaption, annotTool, &iAAnnotationTool::removeAnnotation);
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::addMode, annotTool, &iAAnnotationTool::startAddMode);
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::selectCaption, annotTool, &iAAnnotationTool::focusToAnnotation);
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::hideCaption, annotTool, &iAAnnotationTool::hideAnnotation);
		connect(annotTool, &iAAnnotationTool::focusedToAnnotation, m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::sendInteractionUpdate);

		m_clientList = new QTableWidget(child);
		QStringList columnNames = { "id", "status", "sent", "received" };
		m_clientList->setColumnCount(columnNames.size());
		m_clientList->setHorizontalHeaderLabels(columnNames);
		m_clientList->verticalHeader()->hide();
		//m_clientList->setSelectionBehavior(QAbstractItemView::SelectRows);
		//m_clientList->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
		m_clientList->setEditTriggers(QAbstractItemView::NoEditTriggers);
		m_clientList->resizeColumnsToContents();
		auto dw = new iADockWidgetWrapper(m_clientList, "Clients", "RemoteClientList");
		child->splitDockWidget(child->renderDockWidget(), dw, Qt::Vertical);

		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::clientConnected, this, [this](qulonglong id)
			{
				int row = m_clientList->rowCount();
				m_clientList->insertRow(row);
				m_clientList->setItem(row, 0, new QTableWidgetItem(QString::number(id)));
				m_clientList->setItem(row, 1, new QTableWidgetItem("connected"));
				m_clientList->setItem(row, 2, new QTableWidgetItem(QString::number(0)));
				m_clientList->setItem(row, 3, new QTableWidgetItem(QString::number(0)));
				m_clientList->resizeColumnsToContents();
			});
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::clientDisconnected, this, [this](qulonglong id)
			{
				int row = findClientRow(id);
				m_clientList->item(row, 1)->setText("disconnected");
				m_clientList->resizeColumnsToContents();
			});
		connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::clientTransferUpdated, this, [this](qulonglong id, qulonglong rcvd, qulonglong sent)
			{
				int row = findClientRow(id);
				m_clientList->item(row, 2)->setText(dblToStringWithUnits(sent)+"B");
				m_clientList->item(row, 3)->setText(dblToStringWithUnits(rcvd)+"B");
				m_clientList->resizeColumnsToContents();
			});
	}

private:
	std::unique_ptr<iARemoteRenderer> m_remoteRenderer;
	QMap<QString, QWidget*> m_viewWidgets;
#ifdef QT_HTTPSERVER
	std::unique_ptr<QHttpServer> m_httpServer;
#endif
	QTableWidget* m_clientList;

	int findClientRow(qulonglong clientID)
	{
		for (int row = 0; row < m_clientList->rowCount(); ++row)
		{
			auto listClientID = m_clientList->item(row, 0)->text().toULongLong();
			if (listClientID == clientID)
			{
				return row;
			}
		}
		return -1;
	}
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

	QAction* actionWS = new QAction("WebSocket Server", m_mainWnd);
	connect(actionWS, &QAction::triggered, this, [this]()
		{
			addToolToActiveMdiChild<iAWebSocketServerTool>(iAWebSocketServerTool::Name, m_mainWnd);
		});
	m_mainWnd->makeActionChildDependent(actionWS);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionWS);
}
