// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAUnityWebsocketServerTool.h"

#include "iAPlaneSliceTool.h"

#include <iADataSetRenderer.h>
#include <iADataSetViewer.h>
#include <iADockWidgetWrapper.h>
#include <iAImageData.h>
#include <iAMdiChild.h>
#include <iAToolHelper.h>

#include <iAAABB.h>
#include <iALog.h>
#include <iAStringHelper.h>

#include <vtkMath.h>
#include <vtkProp3D.h>
#include <vtkQuaternion.h>
#include <vtkTransform.h>

#include <QBoxLayout>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QMetaEnum>
#include <QRegularExpression>
#include <QTableWidget>
#include <QThread>
#include <QToolButton>
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
			LOG(lvlError, QString("  %1 outside of valid range 0..%2")
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
		AddTranslation,
		AddScaling,
		AddRotationQuaternion,
		AddRotationEuler,
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
	void readArray(QDataStream& stream, std::array<float, N>& a)
	{
		int expectedBytes = static_cast<int>(sizeof(float) * N);
		auto actualBytes = stream.readRawData(reinterpret_cast<char*>(a.data()), expectedBytes);
		if (actualBytes != expectedBytes)
		{
			LOG(lvlWarn, QString("  Expected %1 but read %2 bytes!").arg(expectedBytes).arg(actualBytes));
		}
	}
	template <std::size_t N>
	void writeArray(QDataStream& stream, std::array<float, N>const & a)
	{
		int expectedBytes = static_cast<int>(sizeof(float) * N);
		auto actualBytes = stream.writeRawData(reinterpret_cast<const char*>(a.data()), expectedBytes);
		if (actualBytes != expectedBytes)
		{
			LOG(lvlWarn, QString("  Expected %1 but wrote %2 bytes!").arg(expectedBytes).arg(actualBytes));
		}
	}

	template <typename T>
	std::array<T, 3> applyRotationToVector(std::array<T, 3> vec, std::array<T, 4> quatData)
	{
		vtkQuaternion<T> quat;
		quat.Set(quatData.data());
		auto normQuat = quat.Normalized();
		std::array<T, 3> result;
		vtkMath::RotateVectorByNormalizedQuaternion(vec.data(), normQuat.GetData(), result.data());
		return result;
	}

	template <typename T>
	std::array<T, 4> getRotationQuaternionFromVectors(std::array<T, 3> vec1, std::array<T, 3> vec2)
	{
		// source: https://public.kitware.com/pipermail/vtkusers/2011-December/071790.html
		T vec[3];
		vtkMath::Cross(vec1, vec2, vec);
		T costheta = vtkMath::Dot(vec1, vec2);
		T sintheta = vtkMath::Norm(vec);
		T theta = atan2(sintheta, costheta);
		if (sintheta != 0)
		{
			vec[0] /= sintheta;
			vec[1] /= sintheta;
			vec[2] /= sintheta;
		}
		// convert to quaternion
		costheta = cos(0.5 * theta);
		sintheta = sin(0.5 * theta);
		std::array<T, 4> quat = {
			costheta,
			vec[0] * sintheta, vec[1] * sintheta, vec[2] * sintheta
		};
		LOG(lvlInfo, QString("Plane: normal=(%2), quat=%3")
			.arg(arrayToString(vec2))
			.arg(arrayToString(quat))
		);
		return quat;
	}

	std::array<float, 3> DefaultPlaneNormal = { 0, 0, 1 };
}

class iAUnityWebsocketServerToolImpl: public QObject
{
private:
	template <std::size_t N>
	void processObjectTransform(QDataStream& rcvStream, quint64 clientID, quint64 objID, ObjectCommandType objCmdType, iAMdiChild* child)
	{
		std::array<float, N> values{};
		readArray(rcvStream, values);
		LOG(lvlInfo, QString("  Object command=%1 received for object ID=%2 with values of %3; broadcasting!")
			.arg(static_cast<int>(objCmdType)).arg(objID).arg(arrayToString(values)));
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream << MessageType::Object << objCmdType << objID;
		writeArray<N>(outStream, values);
		broadcastMsg(outData, clientID);

		if (clientID == m_syncedClientID)	// if sync enabled, apply to local objects:
		{
			// hard-coded object IDs for now:
			if (objID == 0)    // 0 -> Object (dataset)
			{
				auto renderer = child->dataSetViewer(child->firstImageDataSetIdx())->renderer();
				auto prop = renderer->vtkProp();
				switch (objCmdType)
				{
				case ObjectCommandType::SetMatrix:
				{
					vtkNew<vtkTransform> tr;
					assert(N == 16);
					std::array<double, N> matrix;
					std::copy(values.begin(), values.end(), matrix.begin());
					tr->SetMatrix(matrix.data());
					prop->SetUserTransform(tr);
					break;
				}
				case ObjectCommandType::AddScaling:
				{
					double scale[3];
					prop->GetScale(scale);
					for (int i = 0; i < 3; ++i)
					{
						scale[i] += values[i];
					}
					prop->SetScale(scale);
					break;
				}
				case ObjectCommandType::AddTranslation:
				{
					double pos[3];
					prop->GetPosition(pos);
					for (int i = 0; i < 3; ++i)
					{
						pos[i] += values[i];
					}
					prop->SetPosition(pos);
					break;
				}
				case ObjectCommandType::AddRotationEuler:
				{
					double angles[3];
					prop->GetOrientation(angles);
					for (int i = 0; i < 3; ++i)
					{
						angles[i] += values[i];
					}
					prop->SetOrientation(angles);
					break;
				}
				}
			}
			// objID = 1 -> Slicing plane
			else
			{
			//	m_planeSliceTool->setMatrix(values); ???
			}

			child->updateRenderer();
		}
	}

	enum iAClientTableColumn
	{
		ID,
		Address,
		Actions
	};

public:

	iAUnityWebsocketServerToolImpl(iAMainWindow* mainWnd, iAMdiChild* child) :
		m_wsServer(new QWebSocketServer(iAUnityWebsocketServerTool::Name, QWebSocketServer::NonSecureMode, this)),
		m_nextClientID(1),
		m_dataState(DataState::NoDataset),
		m_clientListContainer(new QWidget(child)),
		m_clientTable(new QTableWidget(m_clientListContainer)),
		m_clientListDW(new iADockWidgetWrapper(m_clientListContainer, "Client List", "ClientList")),
		m_syncedClientID(-1)
	{
		m_wsServer->moveToThread(&m_serverThread);
		m_serverThread.start();

		m_planeSliceTool = getTool<iAPlaneSliceTool>(child);
		if (!m_planeSliceTool)
		{
			m_planeSliceTool = addToolToActiveMdiChild<iAPlaneSliceTool>(iAPlaneSliceTool::Name, mainWnd, true);
		}
		using Self = iAUnityWebsocketServerToolImpl;
		connect(m_planeSliceTool, &iAPlaneSliceTool::snapshotAdded,   this, &Self::addSnapshot);
		connect(m_planeSliceTool, &iAPlaneSliceTool::snapshotRemoved, this, &Self::removeSnapshot);
		connect(m_planeSliceTool, &iAPlaneSliceTool::snapshotsCleared, this, &Self::clearSnapshots);

		if (m_wsServer->listen(QHostAddress::Any, 50505))
		{
			LOG(lvlInfo, QString("%1: Listening on %2:%3")
				.arg(iAUnityWebsocketServerTool::Name).arg(m_wsServer->serverAddress().toString()).arg(m_wsServer->serverPort()));

			child->splitDockWidget(child->renderDockWidget(), m_clientListDW, Qt::Vertical);
			m_clientListContainer->setLayout(new QVBoxLayout);
			m_clientListContainer->layout()->setContentsMargins(0, 0, 0, 0);
			m_clientListContainer->layout()->setSpacing(1);
			m_clientListContainer->layout()->addWidget(new QLabel(QString("Listening on %1:%2").
				arg(m_wsServer->serverAddress().toString()).arg(m_wsServer->serverPort())));
			QStringList columnNames = QStringList() << "ID" << "Client address" << "Actions";
			m_clientTable->setColumnCount(static_cast<int>(columnNames.size()));
			m_clientTable->setHorizontalHeaderLabels(columnNames);
			m_clientTable->verticalHeader()->hide();
			//m_clientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
			m_clientTable->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
			m_clientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
			m_clientListContainer->layout()->addWidget(m_clientTable);
			connect(m_wsServer, &QWebSocketServer::newConnection, this,
				[this, child]
			{
				auto client = m_wsServer->nextPendingConnection();
				LOG(lvlInfo, QString("%1: Client connected: %2:%3")
					.arg(iAUnityWebsocketServerTool::Name).arg(client->peerAddress().toString()).arg(client->peerPort()));
				auto clientID = m_nextClientID++;  // simplest possible ID assignment: next free ID. Maybe random?

				int clientRow = m_clientTable->rowCount();
				m_clientTable->insertRow(clientRow);
				auto idItem = new QTableWidgetItem(QString::number(clientID));
				idItem->setData(Qt::UserRole, clientID);
				m_clientTable->setItem(clientRow, iAClientTableColumn::ID, idItem);
				m_clientTable->setItem(clientRow, iAClientTableColumn::Address, new QTableWidgetItem(client->peerAddress().toString()));

				auto w = new QWidget();
				w->setLayout(new QHBoxLayout);
				w->layout()->setContentsMargins(0, 0, 0, 0);
				w->layout()->setSpacing(1);

				auto syncAction = new QAction("Sync");
				syncAction->setToolTip("Synchronize Views between this client and this window.");
				syncAction->setCheckable(true);
				syncAction->setChecked(false);
				m_syncActions.insert(std::make_pair(clientID, syncAction));
				QObject::connect(syncAction, &QAction::toggled, m_clientTable,
					[this, clientID, syncAction]()
					{
						bool checked = syncAction->isChecked();
						m_syncedClientID = (checked) ? clientID : - 1;
						for (auto s : m_syncActions)
						{
							// disable other actions:
							if (s.first == clientID)
							{
								continue;
							}
							QSignalBlocker b(s.second);
							s.second->setChecked(false);
						}
						// trigger sync of last known camera? or just wait for next update...
					});
				iAMainWindow::get()->addActionIcon(syncAction, "update");
				auto syncButton = new QToolButton(w);
				syncButton->setDefaultAction(syncAction);

				auto disconnectAction = new QAction("Sync");
				disconnectAction->setToolTip("Synchronize Views between this client and this window.");
				disconnectAction->setCheckable(true);
				disconnectAction->setChecked(false);
				QObject::connect(disconnectAction, &QAction::triggered, m_clientTable,
				[this, clientID, disconnectAction]()
				{
					m_clientSocket[clientID]->close(QWebSocketProtocol::CloseCodeNormal, "Server user manually requested client disconnect.");
				});
				iAMainWindow::get()->addActionIcon(disconnectAction, "close");
				auto disconnectButton = new QToolButton(w);
				disconnectButton->setDefaultAction(disconnectAction);

				w->layout()->addWidget(syncButton);
				w->layout()->addWidget(disconnectButton);
				m_clientTable->setCellWidget(clientRow, iAClientTableColumn::Actions, w);

				m_clientSocket[clientID] = client;
				m_clientState[clientID] = ClientState::AwaitingProtocolNegotiation;
				connect(client, &QWebSocket::stateChanged, this, [clientID](QAbstractSocket::SocketState state)
				{
					LOG(lvlDebug, QString("%1: Client (ID=%2): socket state changed to %3")
						.arg(iAUnityWebsocketServerTool::Name).arg(clientID)
						.arg(QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(state)));
				});
				connect(client, &QWebSocket::errorOccurred, this, [clientID](QAbstractSocket::SocketError error)
				{
					LOG(lvlDebug, QString("%1: Client (ID=%2): error occurred: %3")
						.arg(iAUnityWebsocketServerTool::Name).arg(clientID)
						.arg(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(error)));
				});
				connect(client, &QWebSocket::pong, this, [clientID](quint64 elapsedTime, const QByteArray& payload)
				{
					LOG(lvlDebug, QString("%1: Client (ID=%2): pong received; elapsed: %3 ms.")
						.arg(iAUnityWebsocketServerTool::Name).arg(clientID)
						.arg(elapsedTime));
				});
				connect(client, &QWebSocket::textMessageReceived, this, [this, clientID](QString message)
				{
					LOG(lvlInfo, QString("%1: Client (ID=%2): TEXT MESSAGE received: %3.")
						.arg(iAUnityWebsocketServerTool::Name).arg(clientID).arg(message));
				});
				connect(client, &QWebSocket::binaryMessageReceived, this, [this, child, clientID](QByteArray rcvData)
				{
					if (rcvData.size() < 1)
					{
						LOG(lvlError, QString("%1: Invalid message of length 0 received from client %2; ignoring message!")
							.arg(iAUnityWebsocketServerTool::Name)
							.arg(clientID));
						return;
					}
					if (!isInEnum<MessageType>(rcvData[0]))
					{
						LOG(lvlError, QString("%1: Invalid message from client %2: invalid type; ignoring message!")
							.arg(iAUnityWebsocketServerTool::Name)
							.arg(clientID));
						return;
					}
					QDataStream rcvStream(&rcvData, QIODevice::ReadOnly);
					MessageType type;
					rcvStream >> type;
					LOG(lvlInfo, QString("%1: Received message of type %2 from client ID=%3:")
						.arg(iAUnityWebsocketServerTool::Name)
						.arg(static_cast<int>(type))
						.arg(clientID));

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
						LOG(lvlError, QString("  Invalid command: missing subcommand; ignoring message!"));
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
								LOG(lvlError, QString("  Invalid command: subcommand not in valid range; ignoring message!"));
								return;
							}
							CommandType subcommand;
							rcvStream >> subcommand;
							if (subcommand == CommandType::Reset)
							{
								LOG(lvlInfo, QString("  Reset received"));
								m_planeSliceTool->clearSnapshots();
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
									LOG(lvlError, QString("    Invalid message: expected length %1, actually read %2 bytes").arg(fnLen).arg(readBytes));
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
								LOG(lvlError, QString("  Invalid object message: subcommand not in valid range; ignoring message!"));
								return;
							}
							ObjectCommandType objCommand;
							rcvStream >> objCommand;
							quint64 objID;
							rcvStream >> objID;
							LOG(lvlInfo, QString("  Object subcommand %1 for ID %2")
								.arg(static_cast<int>(objCommand)).arg(objID));
							switch (objCommand)
							{
							case ObjectCommandType::SetMatrix:
								processObjectTransform<16>(rcvStream, clientID, objID, objCommand, child);
								break;
							case ObjectCommandType::AddTranslation:
								processObjectTransform<3>(rcvStream, clientID, objID, objCommand, child);
								break;
							case ObjectCommandType::AddScaling:
								processObjectTransform<3>(rcvStream, clientID, objID, objCommand, child);
								break;
							case ObjectCommandType::AddRotationQuaternion:
								processObjectTransform<4>(rcvStream, clientID, objID, objCommand, child);
								break;
							case ObjectCommandType::AddRotationEuler:
							{
								quint8 axis;
								rcvStream >> axis;
								float value;
								rcvStream >> value;
								LOG(lvlInfo, QString("  Object command=AddRotation (Euler Angles) received for object ID=%1 with axis=%3, value=%4.")
									.arg(static_cast<int>(objCommand)).arg(objID).arg(axis).arg(value));
								QByteArray outData;
								QDataStream outStream(&outData, QIODevice::WriteOnly);
								outStream << MessageType::Object << objCommand << objID << axis << value;
								broadcastMsg(outData, clientID);
								// TODO: local application!
								break;
							}
							default:
								LOG(lvlWarn, QString("  Object subcommand %1 not implemented!")
									.arg(static_cast<int>(objCommand)));
								break;
							}
							break;
						}
						case MessageType::Snapshot:
						{
							if (!isInEnum<SnapshotCommandType>(rcvData[1]))
							{  // encountered value and valid range output already in isInEnum!
								LOG(lvlError, QString("  Invalid snapshot message: subcommand not in valid range; ignoring message!"));
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
								std::array<float, 4> rotation;
								readArray(rcvStream, rotation);
								info.normal = applyRotationToVector(DefaultPlaneNormal, rotation);
								auto snapshotID = m_planeSliceTool->addSnapshot(info);
								addSnapshot(snapshotID, info);
								break;
							}
							case SnapshotCommandType::Remove:
							{
								quint64 snapshotID;
								rcvStream >> snapshotID;
								m_planeSliceTool->removeSnapshot(snapshotID);
								removeSnapshot(snapshotID);
								break;
							}
							case SnapshotCommandType::ClearAll:
								m_planeSliceTool->clearSnapshots();
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
					m_syncActions.erase(clientID);
					removeTableEntry(m_clientTable, clientID);
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

	void broadcastMsg(QByteArray const& b, quint64 exceptClientID = -1)
	{
		for (auto c : m_clientSocket)
		{
			if (c.first == exceptClientID)
			{
				continue;
			}
			c.second->sendBinaryMessage(b);
		}
	}

	void addSnapshot(quint64 snapshotID, iASnapshotInfo info)
	{
		LOG(lvlInfo, QString("  New snapshot, ID=%1; position=%2, normal=%3")
			.arg(snapshotID).arg(arrayToString(info.position)).arg(arrayToString(info.normal)));
		QByteArray outData;
		QDataStream stream(&outData, QIODevice::WriteOnly);
		stream << MessageType::Snapshot << SnapshotCommandType::Create << snapshotID;
		writeArray(stream, info.position);
		auto quat = getRotationQuaternionFromVectors(DefaultPlaneNormal, info.normal);
		writeArray(stream, quat);
		broadcastMsg(outData);
	}

	void removeSnapshot(quint64 snapshotID)
	{
		LOG(lvlInfo, QString("  Removing snapshot with ID=%1.").arg(snapshotID));
		QByteArray outData;
		QDataStream stream(&outData, QIODevice::WriteOnly);
		stream << MessageType::Snapshot << SnapshotCommandType::Remove << snapshotID;
		broadcastMsg(outData);
	}

	void clearSnapshots()
	{
		LOG(lvlInfo, QString("  Clearing all snapshots."));
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream << MessageType::Snapshot << SnapshotCommandType::ClearAll;
		broadcastMsg(outData);
	}

	void moveSnapshot(quint64 id, iAMoveAxis axis, float value)
	{
		LOG(lvlInfo, QString("  Moving snapshot (ID=%1, axis=%2, value=%3).")
			.arg(id).arg(static_cast<int>(axis)).arg(value));
		m_planeSliceTool->moveSlice(id, axis, value);
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream << MessageType::Snapshot << SnapshotCommandType::ChangeSlicePosition
			<< id << axis << value;
		broadcastMsg(outData);
	}

	void resetObjects()
	{
		// TODO: handle locally...
	}

	void loadDataSet()
	{
		// TODO: handle locally...
	}

	bool dataSetExists(QString const& fileName, iAMdiChild* child)
	{
		auto childFileName = child->dataSet(child->firstImageDataSetIdx())->metaData(iADataSet::FileNameKey).toString();
		return fileName == childFileName || fileName == QFileInfo(childFileName).fileName();
	}

	void sendDataLoadingRequest(QString const & fileName)
	{
		LOG(lvlInfo, QString("  Broadcasting data loading request to all clients; filename: %1").arg(fileName));
		QByteArray fnBytes = fileName.toUtf8();
		quint32 fnLen = static_cast<int>(fnBytes.size());
		m_dataState = DataState::PendingClientAck;
		m_dataSetFileName = fileName;
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream << MessageType::Command << CommandType::LoadDataset << fnLen;
		auto bytesWritten = outStream.writeRawData(fnBytes, fnLen);
		if (bytesWritten != fnLen)
		{
			LOG(lvlWarn, QString("Expected %1 but wrote %2 bytes!").arg(bytesWritten).arg(fnLen));
		}
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
			if (type != MessageType::ACK)
			{
				LOG(lvlError, QString("Invalid state: client (id=%1, state=%2) sent message other than NAK or ACK (type: %3) as answer to dataset loading!")
					.arg(clientID).arg(enumToIntegral(m_clientState[clientID])).arg(static_cast<int>(type)));
				return;
			}
			if (m_clientState[clientID] != ClientState::PendingDatasetAck)
			{
				LOG(lvlError, QString("Invalid state: client sent ACK for dataset loading, but its state (%1) indicates we're not waiting for his ACK (anymore?).")
					.arg(enumToIntegral(m_clientState[clientID])));
				return;
			}
			LOG(lvlInfo, QString("  Client %1 sent ACK answer to dataset loading.").arg(clientID));
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
	std::map<quint64, QAction*> m_syncActions;
	quint64 m_nextClientID;
	QThread m_serverThread;
	//! dataset state: currently a dataset is (1) not loaded (2) being loaded (3) loaded and sent out to clients
	DataState m_dataState;
	QString m_dataSetFileName;
	iAPlaneSliceTool* m_planeSliceTool;

	// for object handling
//	QTableWidget* m_objectTable;

	QWidget* m_clientListContainer;
	QTableWidget* m_clientTable;
	iADockWidgetWrapper* m_clientListDW;
	int m_syncedClientID;
};

const QString iAUnityWebsocketServerTool::Name("UnityWebSocketServer");

std::shared_ptr<iATool> iAUnityWebsocketServerTool::create(iAMainWindow* mainWnd, iAMdiChild* child)
{
	return std::make_shared<iAUnityWebsocketServerTool>(mainWnd, child);
}

iAUnityWebsocketServerTool::iAUnityWebsocketServerTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_impl(std::make_unique<iAUnityWebsocketServerToolImpl>(mainWnd, child))
{
}

iAUnityWebsocketServerTool::~iAUnityWebsocketServerTool()
{
	m_impl->stop();
}
