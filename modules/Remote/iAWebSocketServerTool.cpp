// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAWebSocketServerTool.h"

#include "iAAABB.h"
#include "iALog.h"

#include <iAMdiChild.h>
#include <iADataSetViewer.h>
#include <iADataSetRenderer.h>

#include <vtkMath.h>
#include <vtkProp3D.h>
#include <vtkTransform.h>

#include <QRegularExpression>
#include <QThread>
#include <QWebSocket>
#include <QWebSocketServer>

#include <cmath>

namespace
{
	const int Port = 9042;

	template <typename T> int sgn(T val) {
		return (T(0) < val) - (val < T(0));
	}
}

class iAWSSToolImpl: public QObject
{
public:

	QWebSocketServer* m_wsServer;
	QList<QWebSocket*> m_clients;
	QThread m_serverThread;

	iAWSSToolImpl(int port, iAMdiChild* child):
		m_wsServer(new QWebSocketServer(QStringLiteral("iAWebSocketServer"), QWebSocketServer::NonSecureMode, this))
	{
		m_wsServer->moveToThread(&m_serverThread);
		m_serverThread.start();

		if (m_wsServer->listen(QHostAddress::Any, port))
		{
			LOG(lvlInfo, QString("WebSocketServer listening on %1:%2").arg(m_wsServer->serverAddress().toString()).arg(m_wsServer->serverPort()));
			connect(m_wsServer, &QWebSocketServer::newConnection, this, [this, child]
				{
					QWebSocket* client = m_wsServer->nextPendingConnection();
					LOG(lvlInfo, QString("Client connected: %1:%2").arg(client->peerAddress().toString()).arg(client->peerPort()));
					connect(client, &QWebSocket::textMessageReceived, this, [this, child](QString message)
						{
							//LOG(lvlInfo, QString("WebSocketServer: Text message received: %1").arg(message));
							// TODO: differentiate message protocols, types, ...
							auto values = message.split(",");
							iAVec3d pos { values[0].remove("(").toDouble(), values[1].toDouble(), values[2].remove(")").toDouble() };
							double q[4] = { values[3].remove("(").toDouble(), values[4].toDouble(), values[5].toDouble(), values[6].remove(")").toDouble() };
							double ayterm = 2 * (q[3] * q[1] - q[0] * q[2]);
							double a2[3] = {
								vtkMath::DegreesFromRadians(std::atan2(2 * (q[3] * q[0] + q[1] * q[2]), 1 - 2 * (q[0] * q[0] + q[1] * q[1]))),
								vtkMath::DegreesFromRadians(-vtkMath::Pi() / 2 + 2 * std::atan2( std::sqrt(1 + ayterm), std::sqrt(1 - ayterm))),
								vtkMath::DegreesFromRadians(std::atan2(2 * (q[3] * q[2] + q[0] * q[1]), 1 - 2 * (q[1] * q[1] + q[2] * q[2])))
							};

							for (int a = 0; a < 3; ++a)
							{   // round to nearest X degrees:
								const double RoundDegrees = 2;
								a2[a] = std::round(a2[a] / RoundDegrees) * RoundDegrees;
							}
							auto renderer = child->dataSetViewer(child->firstImageDataSetIdx())->renderer();
							auto prop = renderer->vtkProp();
							LOG(lvlInfo, QString(
									"position: %1, %2, %3; "
									"angle: %4, %5, %6"
								)
								.arg(pos[0]).arg(pos[1]).arg(pos[2])
								.arg(a2[0]).arg(a2[1]).arg(a2[2])
							);

							auto bounds = renderer->bounds();

							pos *= (bounds.maxCorner() - bounds.minCorner()).length() / 2;
							auto center = (bounds.maxCorner() - bounds.minCorner()) / 2;
							vtkNew<vtkTransform> tr;
							tr->PostMultiply();
							tr->Translate(-center[0], -center[1], -center[2] );
							// rotation: order x-z-y, reverse direction of y
							tr->RotateX(a2[0]);
							tr->RotateZ(-a2[1]);
							tr->RotateY(a2[2]);
							// translation: y, z flipped; x, y reversed:
							tr->Translate(center[0] - pos[0], center[1] - pos[2], center[2] + pos[1]);
							prop->SetUserTransform(tr);
							//prop->SetPosition(pos);
							child->updateRenderer();
						});
					connect(client, &QWebSocket::binaryMessageReceived, this, [this](QByteArray message)
						{
							LOG(lvlInfo, QString("WebSocketServer: Binary message received: %1").arg(message));
						});
					connect(client, &QWebSocket::disconnected, this, [this]
						{
							QWebSocket* client = qobject_cast<QWebSocket*>(sender());
							LOG(lvlInfo, QString("Client %1:%2 disconnected!").arg(client->peerAddress().toString()).arg(client->peerPort()));
							m_clients.removeAll(client);
							client->deleteLater();
						});
					m_clients << client;
				});

			//connect(m_pWebSocketServer, &QWebSocketServer::closed, this, );
		}
		else
		{
			LOG(lvlError, QString("WebSocketServer: Listening failed (error: %1)!").arg(m_wsServer->errorString()));
		}
	}

	void stop()
	{
		m_wsServer->close();
		m_serverThread.quit();
		m_serverThread.wait();
		LOG(lvlInfo, "WebSocketServer STOP");
	}
};

const QString iAWebSocketServerTool::Name("WebSocketServer");

iAWebSocketServerTool::iAWebSocketServerTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_impl(std::make_unique<iAWSSToolImpl>(Port, child))
{
}


iAWebSocketServerTool::~iAWebSocketServerTool()
{
	m_impl->stop();
}