// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemoteTool.h"

#include "iARemoteAction.h"
#include "iARemotePortSettings.h"
#include "iARemoteRenderer.h"
#include "iAWebsocketAPI.h"

// iAguibase
#include <iADockWidgetWrapper.h>
#include <iAMdiChild.h>
#include <iAQCropLabel.h>
#include <iARenderer.h>
#include <iASlicer.h>

// base
#include <iALog.h>
#include <iAStringHelper.h>    // for dblToStringWithUnits

// Labelling
#include <iAAnnotationTool.h>

#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <QHeaderView>
#include <QVBoxLayout>
#include <QTableWidget>

#ifdef QT_HTTPSERVER

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHttpServer>
#include <QMimeDatabase>
#include <QTextStream>

namespace
{
	const int StandardWebSocketPort = 1234;

	void addFileToServe(QString path, QString query, QString fileName, QHttpServer* server, quint16 wsPort)
	{
		auto routeCreated = server->route(query, [path, fileName, wsPort](QHttpServerRequest const& request)
			{
				Q_UNUSED(request);
				auto fileToServe = path + "/" + fileName;
				if (!QFile::exists(fileToServe))
				{
					LOG(lvlError, QString("Could not open file to serve (%1) in given path (%2).").arg(fileName).arg(path));
				}
				if (fileName == "scripts.js" || fileName == "main.js")
				{
					// adapted from QHttpServerResponse::fromFile
					QFile file(fileToServe);
					if (!file.open(QFile::ReadOnly))
					{
						return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
					}
					QTextStream s1(&file);
					auto s = s1.readAll();
					s.replace(QString(":%1").arg(StandardWebSocketPort), QString(":%1").arg(wsPort));
					file.close();
					const auto mimeType = QMimeDatabase().mimeTypeForFileNameAndData(fileName, s.toUtf8()).name().toUtf8();
					return QHttpServerResponse(mimeType, s.toUtf8());
				}
				else
				{
					return QHttpServerResponse::fromFile(fileToServe);
				}
			});

		if (!routeCreated)
		{
			LOG(lvlError, QString("Error creating server route for file %1").arg(fileName));
		}
	}

	void addDirectorytoServer(QString path, QHttpServer* server, quint16 wsPort)
	{
		QDir directory(path);
		QStringList files = directory.entryList(QDir::Files);
		addFileToServe(path, "/", "index.html", server, wsPort);
		for (QString filename : files)
		{
			addFileToServe(path, "/" + filename, filename, server, wsPort);
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

iARemoteTool::iARemoteTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_remoteRenderer(std::make_unique<iARemoteRenderer>(getValue(iARemotePortSettings::defaultAttributes(), iARemotePortSettings::RemoteWebSocketPort).toInt()))
#ifdef QT_HTTPSERVER
	, m_httpServer(std::make_unique<QHttpServer>())
#endif
{
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::initSuccess, this, &iARemoteTool::init);
	m_remoteRenderer->start();
}

void iARemoteTool::init()
{
	auto annotTool = getTool<iAAnnotationTool>(m_child);
	if (!annotTool)
	{
		auto newTool = std::make_shared<iAAnnotationTool>(m_mainWindow, m_child);
		m_child->addTool(iAAnnotationTool::Name, newTool);
		annotTool = newTool.get();
	}
	m_remoteRenderer->addRenderWindow(m_child->renderer()->renderWindow(), "3D");
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		m_remoteRenderer->addRenderWindow(m_child->slicer(i)->renderWindow(), slicerModeString(i));
		m_child->slicer(i)->setShowTooltip(false);
	}
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::controlCommand, this, [this]()
		{
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
					else if (curDown && nextDown)
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
				int pos[] = { static_cast<int>(size[0] * action->x), static_cast<int>(size[1] * action->y) };
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
	addDirectorytoServer(path, m_httpServer.get(), m_remoteRenderer->m_wsAPI->serverPort());
	quint16 firstPort = getValue(iARemotePortSettings::defaultAttributes(), iARemotePortSettings::RemoteHTTPPort).toInt();
	quint16 finalPort = connectHttp(m_httpServer.get(), firstPort);
	if (finalPort == 0)
	{
		LOG(lvlError, QString("Could not set up HTTP server (tried ports %1..%2!").arg(firstPort).arg(finalPort));
		return;
	}
	if (m_httpServer->serverPorts().size() != 1 || m_httpServer->serverPorts()[0] != finalPort)
	{
		LOG(lvlError, "Invalid port configuration!");
	}
	LOG(lvlImportant, QString("You can reach the webserver under http://localhost:%1").arg(finalPort));
#endif

	connect(annotTool, &iAAnnotationTool::annotationsUpdated, m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::updateCaptionList);
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::changeCaptionTitle, annotTool, &iAAnnotationTool::renameAnnotation);
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::removeCaption, annotTool, &iAAnnotationTool::removeAnnotation);
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::addMode, annotTool, &iAAnnotationTool::startAddMode);
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::selectCaption, annotTool, &iAAnnotationTool::focusToAnnotation);
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::hideCaption, annotTool, &iAAnnotationTool::toggleAnnotation);
	connect(annotTool, &iAAnnotationTool::focusedToAnnotation, m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::sendInteractionUpdate);

	auto clientContainer = new QWidget();
	clientContainer->setLayout(new QVBoxLayout);
	clientContainer->layout()->setContentsMargins(0, 0, 0, 0);
	clientContainer->layout()->setSpacing(1);
	auto listenStr = QString("Listening (websocket: %1)").arg(m_remoteRenderer->m_wsAPI->listenAddress());
#ifdef QT_HTTPSERVER
	listenStr += QString("; http: localhost:%1").arg(finalPort);
#endif
	clientContainer->layout()->addWidget(new iAQCropLabel(listenStr));
	auto clientList = new QTableWidget(clientContainer);
	QStringList columnNames = { "ID", "Status", "View", "Sent", "Received" };
	clientList->setColumnCount(static_cast<int>(columnNames.size()));
	clientList->setHorizontalHeaderLabels(columnNames);
	clientList->verticalHeader()->hide();
	//clientList->setSelectionBehavior(QAbstractItemView::SelectRows);
	//clientList->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	clientList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	clientList->resizeColumnsToContents();
	clientContainer->layout()->addWidget(clientList);
	auto dw = new iADockWidgetWrapper(clientContainer, "Remote Rendering Clients", "RemoteClientList", "https://github.com/3dct/open_iA/wiki/Remote");
	m_child->splitDockWidget(m_child->renderDockWidget(), dw, Qt::Vertical);
	enum ColIndices { ColID, ColStatus, ColView, ColRcvd, ColSent };
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::clientConnected, this, [clientList](int id)
		{
			int row = clientList->rowCount();
			clientList->insertRow(row);
			clientList->setItem(row, ColID, new QTableWidgetItem(QString::number(id)));
			clientList->setItem(row, ColStatus, new QTableWidgetItem("connected"));
			clientList->setItem(row, ColView, new QTableWidgetItem("unknown"));
			clientList->setItem(row, ColRcvd, new QTableWidgetItem(QString::number(0)));
			clientList->setItem(row, ColSent, new QTableWidgetItem(QString::number(0)));
			clientList->resizeColumnsToContents();
		});
	auto findClientRow = [clientList](int clientID) -> int
		{
			for (int row = 0; row < clientList->rowCount(); ++row)
			{
				auto listClientID = clientList->item(row, ColID)->text().toInt();
				if (listClientID == clientID)
				{
					return row;
				}
			}
			return -1;
		};
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::clientSubscribed, this, [clientList, findClientRow](int clientID, QString viewID)
		{
			int row = findClientRow(clientID);
			clientList->item(row, ColView)->setText(viewID);
			clientList->resizeColumnsToContents();

		});
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::clientDisconnected, this, [clientList, findClientRow](int id)
		{
			int row = findClientRow(id);
			clientList->item(row, ColStatus)->setText("disconnected");
			clientList->resizeColumnsToContents();
		});
	connect(m_remoteRenderer->m_wsAPI.get(), &iAWebsocketAPI::clientTransferUpdated, this, [clientList, findClientRow](int id, quint64 rcvd, quint64 sent)
		{
			int row = findClientRow(id);
			clientList->item(row, ColRcvd)->setText(dblToStringWithUnits(sent) + "B");
			clientList->item(row, ColSent)->setText(dblToStringWithUnits(rcvd) + "B");
			clientList->resizeColumnsToContents();
		});
}


iARemoteTool::~iARemoteTool()
{
#ifdef QT_HTTPSERVER
	removeClosedPort(m_httpServer->serverPorts()[0]);  // m_httpServer is closed automatically through its destructor called later by unique ptr
#endif
}

const QString iARemoteTool::Name("Remote Render Server");
