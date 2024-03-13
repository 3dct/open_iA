// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAUnityWebsocketServerTool.h"

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
	template<typename EnumType>
	constexpr inline decltype(auto) enumToIntegral(EnumType enumValue)
	{
		static_assert(std::is_enum<EnumType>::value, "Enum type required");
		using EnumValueType = std::underlying_type_t<EnumType>;
		return static_cast<EnumValueType>(enumValue);
	}

	template <typename EnumType>
	constexpr inline bool isInEnum(int val)
	{
		static_assert(std::is_enum<EnumType>::value, "Enum type required");
		bool result = 0 <= val && val < enumToIntegral(EnumType::Count);
		if (!result)
		{
			LOG(lvlWarn, QString("%1 outside of valid range 0..%2")
				.arg(val)
				.arg(static_cast<int>(EnumType::Count)));
		}
		return result;
	}

	const quint64 ServerProtocolVersion = 1;

	enum class MessageType: quint8
	{
		ACK,
		NAK,
		ProtocolVersion,
		ClientID,
		Command,
		Object,
		Snapshot,
		// must be last:
		Count
	};

	enum class CommandType : quint8
	{
		Reset,
		LoadDataset,
		// must be last:
		Count
	};

	enum class ObjectCommandType : quint8
	{
		SetMatrix,
		Translate,
		Scale,
		RotateQuaternion,
		RotateEuler,
		// must be last:
		Count
	};

	enum class SnapshotCommandType : quint8
	{
		CreateClient,
		CreateServer,
		Remove,
		ClearAll,
		ChangeSlicePosition,
		// must be last:
		Count
	};

	enum class ClientState
	{
		AwaitingProtocolNegotiation,  // client just connected and hasn't sent a protocol negotiation message with an acceptable protocol version yet
		Idle,                         // currently, no ongoing "special" conversation
		LoadDataset                   // initiated the load of a dataset, server is in the process of checking availability
	};

	void sendMessage(QWebSocket* s, MessageType t)
	{
		// if (t == MessageType::ACK || t == MessageType::NAK)
		QByteArray b;
		b.append(enumToIntegral(t));
		s->sendBinaryMessage(b);
	}
	void sendClientID(QWebSocket* s, quint64 clientID)
	{
		QByteArray b;
		QDataStream stream(&b, QIODevice::WriteOnly);
		stream << enumToIntegral(MessageType::ClientID);
		stream << clientID;    // QDataStream does Big Endian conversions automatically
		s->sendBinaryMessage(b);
	}
}

class iAUnityWebsocketServerToolImpl: public QObject
{
public:

	QWebSocketServer* m_wsServer;
	std::map<quint64, QWebSocket*> m_clientSocket;
	std::map<quint64, ClientState> m_clientState;
	quint64 m_clientID;
	QThread m_serverThread;

	iAUnityWebsocketServerToolImpl(iAMdiChild* child):
		m_wsServer(new QWebSocketServer(iAUnityWebsocketServerTool::Name, QWebSocketServer::NonSecureMode, this)),
		m_clientID(1)
	{
		m_wsServer->moveToThread(&m_serverThread);
		m_serverThread.start();

		if (m_wsServer->listen(QHostAddress::Any))
		{
			LOG(lvlInfo, QString("%1: Listening on %2:%3")
				.arg(iAUnityWebsocketServerTool::Name).arg(m_wsServer->serverAddress().toString()).arg(m_wsServer->serverPort()));
			connect(m_wsServer, &QWebSocketServer::newConnection, this, [this, child]
			{
				auto client = m_wsServer->nextPendingConnection();
				LOG(lvlInfo, QString("%1: Client connected: %2:%3")
					.arg(iAUnityWebsocketServerTool::Name).arg(client->peerAddress().toString()).arg(client->peerPort()));
				auto clientID = m_clientID++;  // simplest possible ID assignment: next free ID. Maybe random?
				m_clientSocket[clientID] = client;
				m_clientState[clientID] = ClientState::AwaitingProtocolNegotiation;
				connect(client, &QWebSocket::textMessageReceived, this, [this, child](QString message)
				{
					//LOG(lvlInfo, QString("%1: Text message received: %2").arg(iAWebSocketServerTool::Name).arg(message));
				});
				connect(client, &QWebSocket::binaryMessageReceived, this, [this, child, clientID](QByteArray data)
				{
					if (data.size() < 1)
					{
						LOG(lvlError, QString("Invalid message of length 0 received from client %1; ignoring message!").arg(clientID));
						return;
					}
					if (!isInEnum<MessageType>(data[0]))
					{
						LOG(lvlError, QString("Invalid message from client %1: invalid type; ignoring message!").arg(clientID));
						return;
					}
					QDataStream stream(&data, QIODevice::ReadOnly);
					MessageType type;
					stream >> type;
					LOG(lvlInfo, QString("Received message of type %1").arg(static_cast<int>(type)));
					switch (m_clientState[clientID])
					{
					case ClientState::AwaitingProtocolNegotiation:
					{
						if (type != MessageType::ProtocolVersion)
						{
							LOG(lvlError, QString("Invalid message from client %1: Awaiting protocol negotiation message, but got message of type %2; ignoring message!")
								.arg(clientID)
								.arg(data[0]));
							return;
						}
						quint64 clientProtocolVersion;
						stream >> clientProtocolVersion;
						if (clientProtocolVersion > ServerProtocolVersion)
						{
							LOG(lvlWarn, QString("Client advertised unsupported protocol version %1 (we only support up to version %2), sending NAK...")
								.arg(clientProtocolVersion)
								.arg(ServerProtocolVersion));
							sendMessage(m_clientSocket[clientID], MessageType::NAK);
						}
						else
						{
							LOG(lvlInfo, QString("Client advertised supported protocol version %1, sending ACK...")
								.arg(clientProtocolVersion));
							sendMessage(m_clientSocket[clientID], MessageType::ACK);
							sendClientID(m_clientSocket[clientID], clientID);
							m_clientState[clientID] = ClientState::Idle;
						}
						break;
					}
					case ClientState::Idle:
					{
						switch (type)
						{
						case MessageType::Command:
						{
							if (data.size() < 2)
							{
								LOG(lvlInfo, QString("Invalid command: missing subcommand; ignoring message!"));
								return;
							}
							if (!isInEnum<CommandType>(data[1]))
							{   // encountered value and valid range output already in isInEnum!
								LOG(lvlInfo, QString("Invalid command: subcommand not in valid range; ignoring message!"));
								return;
							}
							auto subcommand = static_cast<CommandType>(data[1]);
							if (subcommand == CommandType::Reset)
							{
								LOG(lvlInfo, QString("  Reset"));
							}
							else // CommandType::LoadDataset:
							{
								LOG(lvlInfo, QString("  Sub-Command: %1").arg(subcommand == CommandType::Reset ? "Reset" : "Load Dataset"));
							}
						}
						case MessageType::Object:
						{
							break;
						}
						case MessageType::Snapshot:
						{
							break;
						}
						}
						// react on:
						//    - Reset
						//    - Load Dataset
						//    - Object manipulation
						//    - 
						break;
					}
					case ClientState::LoadDataset:
					{
						break;
					}
					}

					/*
					auto obj  = static_cast<int>(data[1]);
					// TODO: differentiate message protocols, types, ...

					iAVec3d pos;
					using FloatType = float;
					const size_t PosOfs = 2;
					const size_t FloatSize = sizeof(FloatType);
					const size_t PosSize = 3;
					const size_t QuatOfs = PosOfs + PosSize * FloatSize;
					const size_t QuatSize = 4;
					for (size_t i = 0; i < PosSize; ++i)
					{
						pos[i] = extractT<float>(data, PosOfs + i * FloatSize);
					}
					double q[QuatSize];
					for (size_t i = 0; i < QuatSize; ++i) 
					{
						q[i] = extractT<float>(data, QuatOfs + i * FloatSize);
					}
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
						.arg(iAUnityWebsocketServerTool::Name)
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
					*/
				});
				connect(client, &QWebSocket::disconnected, this, [this, client, clientID]
				{
					//QWebSocket* client = qobject_cast<QWebSocket*>(sender());
					LOG(lvlInfo, QString("%1: Client (ID=%2, %3:%4) disconnected!")
						.arg(iAUnityWebsocketServerTool::Name)
						.arg(clientID)
						.arg(client->peerAddress().toString()).arg(client->peerPort()));
					m_clientSocket.erase(clientID);
					m_clientState.erase(clientID);
					client->deleteLater();
				});
			});
		}
		else
		{
			LOG(lvlError, QString("%1: Listening failed (error: %2)!").arg(iAUnityWebsocketServerTool::Name).arg(m_wsServer->errorString()));
		}
	}

	void stop()
	{
		m_wsServer->close();
		m_serverThread.quit();
		m_serverThread.wait();
		LOG(lvlInfo, QString("%1 STOP").arg(iAUnityWebsocketServerTool::Name));
	}
};

const QString iAUnityWebsocketServerTool::Name("UnityWebSocketServer");

iAUnityWebsocketServerTool::iAUnityWebsocketServerTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_impl(std::make_unique<iAUnityWebsocketServerToolImpl>(child))
{
}

iAUnityWebsocketServerTool::~iAUnityWebsocketServerTool()
{
	m_impl->stop();
}