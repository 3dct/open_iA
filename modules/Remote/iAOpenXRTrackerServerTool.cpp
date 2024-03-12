// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAOpenXRTrackerServerTool.h"

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
	float toFloat(QByteArray const& arr, size_t ofs)
	{
		//static_assert(std::numeric_limits<float>::is_iec559, "Only supports IEC 559 (IEEE 754) float");
		const float* val = reinterpret_cast<const float*>(&arr.constData()[ofs]);
		return *val;
	}
}

class iAOpenXRTrackerServerToolImpl: public QObject
{
public:

	QWebSocketServer* m_wsServer;
	QList<QWebSocket*> m_clients;
	QThread m_serverThread;

	iAOpenXRTrackerServerToolImpl(iAMdiChild* child):
		m_wsServer(new QWebSocketServer(iAOpenXRTrackerServerTool::Name, QWebSocketServer::NonSecureMode, this))
	{
		m_wsServer->moveToThread(&m_serverThread);
		m_serverThread.start();

		if (m_wsServer->listen(QHostAddress::Any))
		{
			LOG(lvlInfo, QString("%1: Listening on %2:%3")
				.arg(iAOpenXRTrackerServerTool::Name).arg(m_wsServer->serverAddress().toString()).arg(m_wsServer->serverPort()));
			connect(m_wsServer, &QWebSocketServer::newConnection, this, [this, child]
			{
				QWebSocket* client = m_wsServer->nextPendingConnection();
				LOG(lvlInfo, QString("%1: Client connected: %2:%3")
					.arg(iAOpenXRTrackerServerTool::Name).arg(client->peerAddress().toString()).arg(client->peerPort()));
				connect(client, &QWebSocket::textMessageReceived, this, [this, child](QString message)
				{
					//LOG(lvlInfo, QString("%1: Text message received: %2").arg(iAWebSocketServerTool::Name).arg(message));
				});
				connect(client, &QWebSocket::binaryMessageReceived, this, [this, child](QByteArray data)
				{
					auto type = static_cast<int>(data[0]);
					auto obj  = static_cast<int>(data[1]);
					// TODO: differentiate message protocols, types, ...

					iAVec3d pos;
					using FloatType = float;
					const size_t PosOfs = 2;
					const size_t FloatSize = sizeof(FloatType);
					const size_t PosSize = 3;
					const size_t QuatOfs = PosOfs + PosSize * FloatSize;
					const size_t QuatSize = 4;
					for (size_t i = 0; i < PosSize; ++i) { pos[i] = toFloat(data, PosOfs + i * FloatSize); }
					double q[QuatSize];
					for (size_t i = 0; i < QuatSize; ++i) { q[i] = toFloat(data, QuatOfs + i * FloatSize); }
					double ayterm = 2 * (q[3] * q[1] - q[0] * q[2]);
					double a2[3] = {
						vtkMath::DegreesFromRadians(std::atan2(2 * (q[3] * q[0] + q[1] * q[2]), 1 - 2 * (q[0] * q[0] + q[1] * q[1]))),
						vtkMath::DegreesFromRadians(-vtkMath::Pi() / 2 + 2 * std::atan2(std::sqrt(1 + ayterm), std::sqrt(1 - ayterm))),
						vtkMath::DegreesFromRadians(std::atan2(2 * (q[3] * q[2] + q[0] * q[1]), 1 - 2 * (q[1] * q[1] + q[2] * q[2])))
					};

					for (int a = 0; a < 3; ++a)
					{   // round to nearest X degrees:
						const double RoundDegrees = 2;
						a2[a] = std::round(a2[a] / RoundDegrees) * RoundDegrees;
					}
					auto renderer = child->dataSetViewer(child->firstImageDataSetIdx())->renderer();
					auto prop = renderer->vtkProp();

					LOG(lvlInfo, QString("%1: Binary message received "
						"(type: %2; obj: %3; pos: (%4, %5, %6); ")
						.arg(iAOpenXRTrackerServerTool::Name)
						.arg(type)
						.arg(obj)
						.arg(pos[0]).arg(pos[1]).arg(pos[2])
					);
					LOG(lvlInfo, QString("    rot: (%1, %2, %3, %4), angle: (%5, %6, %7)")
						.arg(q[0]).arg(q[1]).arg(q[2]).arg(q[3])
						.arg(a2[0]).arg(a2[1]).arg(a2[2]));

					auto bounds = renderer->bounds();
					pos *= (bounds.maxCorner() - bounds.minCorner()).length() / 2;
					auto center = (bounds.maxCorner() - bounds.minCorner()) / 2;

					vtkNew<vtkTransform> tr;
					tr->PostMultiply();
					tr->Translate(-center[0], -center[1], -center[2]);
					// rotation: order x-z-y, reverse direction of y
					tr->RotateX(a2[0]);
					tr->RotateZ(-a2[1]);
					tr->RotateY(a2[2]);
					// translation: y, z flipped; x, y reversed:
					tr->Translate(center[0] - pos[0], center[1] - pos[2], center[2] + pos[1]);
					prop->SetUserTransform(tr);
					child->updateRenderer();
				});
				connect(client, &QWebSocket::disconnected, this, [this]
				{
					QWebSocket* client = qobject_cast<QWebSocket*>(sender());
					LOG(lvlInfo, QString("%1: Client %2:%3 disconnected!").arg(iAOpenXRTrackerServerTool::Name).arg(client->peerAddress().toString()).arg(client->peerPort()));
					m_clients.removeAll(client);
					client->deleteLater();
				});
				m_clients << client;
			});
		}
		else
		{
			LOG(lvlError, QString("%1: Listening failed (error: %2)!").arg(iAOpenXRTrackerServerTool::Name).arg(m_wsServer->errorString()));
		}
	}

	void stop()
	{
		m_wsServer->close();
		m_serverThread.quit();
		m_serverThread.wait();
		LOG(lvlInfo, QString("%1 STOP").arg(iAOpenXRTrackerServerTool::Name));
	}
};

const QString iAOpenXRTrackerServerTool::Name("OpenXRTracker");

iAOpenXRTrackerServerTool::iAOpenXRTrackerServerTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_impl(std::make_unique<iAOpenXRTrackerServerToolImpl>(child))
{
}

iAOpenXRTrackerServerTool::~iAOpenXRTrackerServerTool()
{
	m_impl->stop();
}