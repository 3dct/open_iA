// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAUnityWebsocketServerTool.h"

#include "iAPlaneSliceTool.h"

#include <iADataSetRenderer.h>
#include <iADataSetViewer.h>
#include <iAImageData.h>
#include <iAMdiChild.h>
#include <iAToolHelper.h>

#include <iAAABB.h>
#include <iALog.h>

#include <vtkMath.h>
#include <vtkProp3D.h>
#include <vtkTransform.h>

#include <QFileInfo>
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
			LOG(lvlError, QString("%1 outside of valid range 0..%2")
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
		Create,  // Reserved, not yet implemented
		Remove,  // Reserved, not yet implemented
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
		Create,
		Remove,
		ClearAll,
		ChangeSlicePosition,
		// must be last:
		Count
	};

	enum class ClientState : int
	{
		AwaitingProtocolNegotiation,  // client just connected and hasn't sent a protocol negotiation message with an acceptable protocol version yet
		Idle,                         // currently, no ongoing "special" conversation
		PendingDatasetAck,            // load dataset has been initiated; server has checked availability on his end and has sent out load dataset messages
		DatasetAcknowledged           // client has confirmed availability of dataset
	};

	enum class DataState : int
	{
		NoDataset,
		PendingClientAck,
		//LoadingDataset,
		DatasetLoaded
	};

	enum class ObjectID : quint64
	{
		Dataset,
		SlicingPlane
		//Camera
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
	template <std::size_t N>
	void readArray(QDataStream& stream, std::array<float, N> a)
	{
		stream.readRawData(reinterpret_cast<char*>(a.data()), sizeof(float) * N);
	}
	template <std::size_t N>
	void writeArray(QDataStream& stream, std::array<float, N> a)
	{
		stream.writeRawData(reinterpret_cast<char*>(a.data()), sizeof(float) * N);
	}
}

class iAUnityWebsocketServerToolImpl: public QObject
{
public:

	iAUnityWebsocketServerToolImpl(iAMainWindow* mainWnd, iAMdiChild* child) :
		m_wsServer(new QWebSocketServer(iAUnityWebsocketServerTool::Name, QWebSocketServer::NonSecureMode, this)),
		m_nextClientID(1)
	{
		m_wsServer->moveToThread(&m_serverThread);
		m_serverThread.start();

		m_planeSliceTool = getTool<iAPlaneSliceTool>(child);
		if (!m_planeSliceTool)
		{
			m_planeSliceTool = addToolToActiveMdiChild<iAPlaneSliceTool>(iAPlaneSliceTool::Name, mainWnd, true);
		}

		if (m_wsServer->listen(QHostAddress::Any, 50505))
		{
			LOG(lvlInfo, QString("%1: Listening on %2:%3")
				.arg(iAUnityWebsocketServerTool::Name).arg(m_wsServer->serverAddress().toString()).arg(m_wsServer->serverPort()));
			connect(m_wsServer, &QWebSocketServer::newConnection, this,
				[this, child]
			{
				auto client = m_wsServer->nextPendingConnection();
				LOG(lvlInfo, QString("%1: Client connected: %2:%3")
					.arg(iAUnityWebsocketServerTool::Name).arg(client->peerAddress().toString()).arg(client->peerPort()));
				auto clientID = m_nextClientID++;  // simplest possible ID assignment: next free ID. Maybe random?
				m_clientSocket[clientID] = client;
				m_clientState[clientID] = ClientState::AwaitingProtocolNegotiation;
				connect(client, &QWebSocket::textMessageReceived, this, [this, child](QString message)
				{
					LOG(lvlInfo, QString("%1: TEXT MESSAGE received: %2").arg(iAUnityWebsocketServerTool::Name).arg(message));
				});
				connect(client, &QWebSocket::binaryMessageReceived, this, [this, child, clientID](QByteArray rcvData)
				{
					if (rcvData.size() < 1)
					{
						LOG(lvlError, QString("Invalid message of length 0 received from client %1; ignoring message!").arg(clientID));
						return;
					}
					if (!isInEnum<MessageType>(rcvData[0]))
					{
						LOG(lvlError, QString("Invalid message from client %1: invalid type; ignoring message!").arg(clientID));
						return;
					}
					QDataStream rcvStream(&rcvData, QIODevice::ReadOnly);
					MessageType type;
					rcvStream >> type;
					LOG(lvlInfo, QString("Received message of type %1").arg(static_cast<int>(type)));

					if (m_clientState[clientID] == ClientState::AwaitingProtocolNegotiation)
					{
						handleVersionNegotiation(type, clientID, rcvStream); // no need to handle return type (yet)
						return;
					}
					// when waiting on feedback for load dataset message, only (above) protocol negotiation for new clients is permitted
					if (m_dataState == DataState::PendingClientAck)
					{
						if (type != MessageType::NAK && type != MessageType::ACK)
						{
							LOG(lvlError, QString("  Invalid message from client %1: We are currently waiting for client acknowledgements. "
								"Only ACK or NAK are permissible; ignoring message!").arg(clientID));
							return;
							// current protocol: (we must) silently ignore:
							//     NAK is only permitted as response to protocol negotiation or as part of dataset loading procedure
						}
						handleClientDataResponse(clientID, type);
						return;
					}

					if (rcvData.size() < 2)
					{
						LOG(lvlInfo, QString("Invalid command: missing subcommand; ignoring message!"));
						return;
					}
					switch (m_clientState[clientID])
					{
					case ClientState::Idle:
					{
						switch (type)
						{
						case MessageType::Command:
						{
							if (!isInEnum<CommandType>(rcvData[1]))
							{   // encountered value and valid range output already in isInEnum!
								LOG(lvlInfo, QString("Invalid command: subcommand not in valid range; ignoring message!"));
								return;
							}
							CommandType subcommand;
							rcvStream >> subcommand;
							if (subcommand == CommandType::Reset)
							{
								LOG(lvlInfo, QString("  Reset received"));

								clearSnapshots();
								resetObjects();
							}
							else // CommandType::LoadDataset:
							{
								LOG(lvlInfo, QString("  Load Dataset received"));
								// https://forum.qt.io/topic/89832/reading-char-array-from-qdatastream
								quint32 fnLen;
								rcvStream >> fnLen;
								QByteArray fnBytes;
								fnBytes.resize(fnLen);
								auto readBytes = rcvStream.readRawData(fnBytes.data(), fnLen);
								if (static_cast<quint32>(readBytes) != fnLen)
								{
									LOG(lvlInfo, QString("    Invalid message: expected length %1, actually read %2 bytes").arg(fnLen).arg(readBytes));
									return;
								}
								QString fileName = QString::fromUtf8(fnBytes);
								LOG(lvlInfo, QString("  Client requested loading dataset from filename '%1' (len=%2).").arg(fileName).arg(fnLen));

								if (!dataSetExists(fileName, child))
								{
									LOG(lvlWarn, "  Requested dataset does not exist, sending NAK!");
									sendMessage(m_clientSocket[clientID], MessageType::NAK);
									return;
								}
								sendDataLoadingRequest(fileName);
							}
							return;
						}
						case MessageType::Object:
						{
							if (!isInEnum<ObjectCommandType>(rcvData[1]))
							{  // encountered value and valid range output already in isInEnum!
								LOG(lvlInfo,
									QString("Invalid object message: subcommand not in valid range; ignoring message!"));
								return;
							}
							ObjectCommandType objCommand;
							rcvStream >> objCommand;
							quint64 objID;
							rcvStream >> objID;
							LOG(lvlInfo, QString("Object subcommand %1 for ID %2")
								.arg(static_cast<int>(objCommand)).arg(objID));
							switch (objCommand)
							{
							case ObjectCommandType::SetMatrix:
								break;
							case ObjectCommandType::Translate:
								break;
							case ObjectCommandType::Scale:
								break;
							case ObjectCommandType::RotateQuaternion:
								break;
							case ObjectCommandType::RotateEuler:
								break;
							default:
								LOG(lvlWarn, QString("Object subcommand %1 not implemented!")
									.arg(static_cast<int>(objCommand)));
								break;
							}
							break;
						}
						case MessageType::Snapshot:
						{
							if (!isInEnum<SnapshotCommandType>(rcvData[1]))
							{  // encountered value and valid range output already in isInEnum!
								LOG(lvlInfo,
									QString(
										"Invalid object message: subcommand not in valid range; ignoring message!"));
								return;
							}
							SnapshotCommandType snapshotCommand;
							rcvStream >> snapshotCommand;
							switch (snapshotCommand)
							{
							case SnapshotCommandType::Create:
							{
								iASnapshotInfo info{};
								readArray(rcvStream, info.position);
								readArray(rcvStream, info.rotation);
								addSnapshot(info);
								break;
							}
							case SnapshotCommandType::Remove:
							{
								quint64 snapshotID;
								rcvStream >> snapshotID;
								removeSnapshot(snapshotID);
								break;
							}
							case SnapshotCommandType::ClearAll:
								clearSnapshots();
								break;
							case SnapshotCommandType::ChangeSlicePosition:
							{
								quint64 snapshotID;
								iAMoveAxis axis;
								float value;
								rcvStream >> snapshotID;
								rcvStream >> axis;
								rcvStream >> value;
								moveSnapshot(snapshotID, axis, value);
								break;
							}
							}
							break;
						}
						}
						break;
					}/*
					 // implicitly handled through m_dataState code above switch
					case ClientState::PendingDatasetAck:
					{
						break;
					}
					*/
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
					if (m_dataState == DataState::PendingClientAck) // special handling for if a dataset loading procedure is pending:
					{
						if (!m_clientSocket.empty())
						{   // if there are other clients connected, check if now all (still connected) clients have sent their ACK
							checkAllClientConfirmedData();
						}
						else
						{   // if the disconnected client was the last one, simply switch to "No dataset loaded" mode
							m_dataState = DataState::NoDataset;
							m_dataSetFileName = "";
						}
					}
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

private:

	void broadcastMsg(QByteArray const& b)
	{
		for (auto c : m_clientSocket)
		{
			c.second->sendBinaryMessage(b);
		}
	}

	void addSnapshot(iASnapshotInfo info)
	{
		auto snapshotID = m_planeSliceTool->addSnapshot(info);
		LOG(lvlWarn, QString("  New snapshot, ID=%1.").arg(snapshotID));
		QByteArray outData;
		QDataStream stream(&outData, QIODevice::WriteOnly);
		stream << MessageType::Snapshot << SnapshotCommandType::Create << snapshotID;
		writeArray(stream, info.position);
		writeArray(stream, info.rotation);
		broadcastMsg(outData);
	}

	void removeSnapshot(quint64 snapshotID)
	{
		LOG(lvlWarn, QString("  Removing snapshot with ID=%1.").arg(snapshotID));
		m_planeSliceTool->removeSnapshot(snapshotID);
		QByteArray outData;
		QDataStream stream(&outData, QIODevice::WriteOnly);
		stream << MessageType::Snapshot << SnapshotCommandType::Remove << snapshotID;
		broadcastMsg(outData);
	}

	void clearSnapshots()
	{
		LOG(lvlWarn, QString("  Clearing all snapshots."));
		m_planeSliceTool->clearSnapshots();
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream << MessageType::Command << SnapshotCommandType::ClearAll;
		broadcastMsg(outData);
	}

	void moveSnapshot(quint64 id, iAMoveAxis axis, float value)
	{
		LOG(lvlWarn, QString("  Moving snapshot (ID=%1, axis=%2, value=%3).")
			.arg(id).arg(static_cast<int>(axis)).arg(value));
		m_planeSliceTool->moveSlice(id, axis, value);
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream << MessageType::Command << SnapshotCommandType::ChangeSlicePosition
			<< id << axis << value;
		broadcastMsg(outData);
	}

	void resetObjects()
	{

	}

	void loadDataSet()
	{

	}

	bool dataSetExists(QString const& fileName, iAMdiChild* child)
	{
		auto childFileName = child->dataSet(child->firstImageDataSetIdx())->metaData(iADataSet::FileNameKey).toString();
		return fileName == childFileName || fileName == QFileInfo(childFileName).fileName();
	}

	void sendDataLoadingRequest(QString const & fileName)
	{
		LOG(lvlWarn, QString("  Broadcasting data loading request to all clients; filename: %1").arg(fileName));
		QByteArray fnBytes = fileName.toUtf8();
		quint32 fnLen = fnBytes.size();
		m_dataState = DataState::PendingClientAck;
		m_dataSetFileName = fileName;
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream << MessageType::Command << CommandType::LoadDataset << fnLen;
		outStream.writeRawData(fnBytes, fnLen);
		broadcastMsg(outData);
		for (auto c : m_clientSocket)
		{
			m_clientState[c.first] = ClientState::PendingDatasetAck;
		}
	}

	void handleClientDataResponse(quint64 clientID, MessageType type)
	{
		if (type == MessageType::NAK)
		{
			LOG(lvlInfo, QString("  Received NAK from client %1, aborting data loading by broadcasting NAK.").arg(clientID));
			// broadcast NAK:
			for (auto s : m_clientSocket)
			{
				sendMessage(s.second, MessageType::NAK);
				m_clientState[s.first] = ClientState::Idle;
			}
			m_dataState = DataState::NoDataset;  // maybe switch back to previous dataset if there is any?
			m_dataSetFileName = "";
		}
		else
		{
			if (m_clientState[clientID] != ClientState::PendingDatasetAck)
			{
				LOG(lvlError, QString("Invalid state: client sent ACK for dataset loading, but its state (%1) indicates we're not waiting for his ACK (anymore?).")
					.arg(enumToIntegral(m_clientState[clientID])));
				return;
			}
			m_clientState[clientID] = ClientState::DatasetAcknowledged;
			checkAllClientConfirmedData();
		}
	}

	void checkAllClientConfirmedData()
	{
		if (m_dataState != DataState::PendingClientAck)
		{
			LOG(lvlError, QString("  Invalid state: check for whether all clients confirmed dataset loading was triggered outside of dataset loading procedure (data state: %1)").arg(enumToIntegral(m_dataState)));
			return;
		}
		bool allACK = true;
		for (auto state : m_clientState)
		{
			if (state.second != ClientState::DatasetAcknowledged)
			{
				allACK = false;
				LOG(lvlInfo, QString("  Not all clients have sent an ACK yet - missing at least one from %1.").arg(state.first));
				break;
			}
		}
		if (allACK)
		{   // send ACK to all clients, finally load dataset
			LOG(lvlInfo, QString("  All clients have ACK'ed the dataset loading; ready to go ahead with actually loading the data, broadcasting ACK!"));
			for (auto s : m_clientSocket)
			{
				sendMessage(s.second, MessageType::ACK);
				m_clientState[s.first] = ClientState::Idle;
			}
			// TODO: actual data loading
			loadDataSet();
			m_dataState = DataState::DatasetLoaded;
		}
	}

	void handleVersionNegotiation(MessageType type, quint64 clientID, QDataStream& stream)
	{
		if (type != MessageType::ProtocolVersion)
		{
			LOG(lvlError, QString("Invalid message from client %1: Awaiting protocol negotiation message, but got message of type %2; ignoring message!")
				.arg(clientID)
				.arg(static_cast<quint8>(type)));
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

			if (m_dataState == DataState::DatasetLoaded)
			{
				// send dataset info ? -> m_dataSetFileName
				// requires separate "state" where we just wait for the ACK of this one client
			}
		}
	}

	QWebSocketServer* m_wsServer;
	std::map<quint64, QWebSocket*> m_clientSocket;
	std::map<quint64, ClientState> m_clientState;
	quint64 m_nextClientID;
	QThread m_serverThread;
	//! dataset state: currently a dataset is (1) not loaded (2) being loaded (3) loaded and sent out to clients
	DataState m_dataState;
	QString m_dataSetFileName;
	iAPlaneSliceTool* m_planeSliceTool;
};

const QString iAUnityWebsocketServerTool::Name("UnityWebSocketServer");

iAUnityWebsocketServerTool::iAUnityWebsocketServerTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_impl(std::make_unique<iAUnityWebsocketServerToolImpl>(mainWnd, child))
{
}

iAUnityWebsocketServerTool::~iAUnityWebsocketServerTool()
{
	m_impl->stop();
}