// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAWebSocketServerTool.h"

#include "iALog.h"

#include <iAMdiChild.h>
#include <iADataSetViewer.h>
#include <iADataSetRenderer.h>

#include <vtkMath.h>
#include <vtkProp3D.h>

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
							LOG(lvlInfo, QString("WebSocketServer: Text message received: %1").arg(message));
							// TODO: differentiate message protocols, types, ...
							//QRegularExpression re("Tracker Position: \\((-?\\d+[.]\\d+), (-?\\d+[.]\\d+), (-?\\d+[.]\\d+)\\); rotation: \\((-?\\d+[.]\\d+), (-?\\d+[.]\\d+), (-?\\d+[.]\\d+), (-?\\d+[.]\\d+)\\)");
							//auto match = re.match(message);
							//double pos[3] = { match.captured(0).toDouble(), match.captured(1).toDouble(), match.captured(2).toDouble() };
							//double angle[3] = { match.captured(3).toDouble(), match.captured(4).toDouble(), match.captured(5).toDouble() };
							auto values = message.split(",");
							double pos[3] = { values[0].remove("(").toDouble(), values[1].toDouble(), values[2].remove(")").toDouble()};
							double angle[3] = { values[3].remove("(").toDouble(), values[4].toDouble(), values[5].remove(")").toDouble() };
							//double x = match.captured(3).toDouble(),
							//	y = match.captured(4).toDouble(),
							//	z = match.captured(5).toDouble(),
							//	w = match.captured(6).toDouble();
							//double sinr_cosp = 2 * (w * x + y * z);
							//double cosr_cosp = 1 - 2 * (x * x + y * y);
							//double angleX = std::atan2(sinr_cosp, cosr_cosp);
							//double sinp = 2 * (w * y - z * x);
							//double angleY = (std::abs(sinp) >= 1)
							//	? vtkMath::Pi() / 2 * sgn(sinp) // use 90 degrees if out of range
							//	: std::asin(sinp);
							//double siny_cosp = 2 * (w * z + x * y);
							//double cosy_cosp = 1 - 2 * (y * y + z * z);
							//double angleZ = std::atan2(siny_cosp, cosy_cosp);
							auto prop = child->dataSetViewer(child->firstImageDataSetIdx())->renderer()->vtkProp();
							LOG(lvlInfo, QString("position: %1, %2, %3; angle: %4, %5, %6")
								.arg(pos[0]).arg(pos[1]).arg(pos[2])
								.arg(angle[0]).arg(angle[1]).arg(angle[2]));
							prop->SetOrientation(angle[0], angle[1], angle[2]);
							prop->SetPosition(pos);
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
	}

	void stop()
	{
		m_wsServer->close();
		m_serverThread.quit();
		m_serverThread.wait();
	}
};

const QString iAWebSocketServerTool::Name("WebSocketServer");

iAWebSocketServerTool::iAWebSocketServerTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_impl(std::make_unique<iAWSSToolImpl>(Port, child))
{
	LOG(lvlInfo, "WebSocketServer START");
}


iAWebSocketServerTool::~iAWebSocketServerTool()
{
	m_impl->stop();
	LOG(lvlInfo, "WebSocketServer STOP");
}