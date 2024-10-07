// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAUnityWebsocketServerTool.h"

#include "iAPlaneSliceTool.h"
#include "iARemotePortSettings.h"

#include <iACameraVis.h>
#include <iADataSetRenderer.h>
#include <iADataSetViewer.h>
#include <iADockWidgetWrapper.h>
#include <iAImageData.h>
#include <iAMdiChild.h>
#include <iARenderer.h>
#include <iAToolHelper.h>

#include <iAAABB.h>
#include <iAColorTheme.h>
#include <iALog.h>
#include <iAStringHelper.h>

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkProp3D.h>
#include <vtkQuaternion.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

#include <QBoxLayout>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QMetaEnum>
#include <QMessageBox>
#include <QTableWidget>
#include <QThread>
#include <QToolButton>
#include <QWebSocket>
#include <QWebSocketServer>

#include <cmath>
#include <limits>

enum class ClientState : int
{
	AwaitingProtocolNegotiation,  // client just connected and hasn't sent a protocol negotiation message with an acceptable protocol version yet
	Idle,                         // currently, no ongoing "special" conversation
	PendingDatasetAck,            // load dataset has been initiated; server has checked availability on his end and has sent out load dataset messages
	PendingSingleClientDatasetAck,// this client has connected when a dataset was loaded, and it should now load this dataset to achieve a synced state
	DatasetAcknowledged           // client has confirmed availability of dataset
};

enum class DataState : int
{
	NoDataset,
	PendingClientAck,
	//LoadingDataset,
	DatasetLoaded
};

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
		SetTranslation,
		SetScaling,
		SetRotationQuaternion,
		SetRotationEuler,
		SetRotationNormalUp,
		// must be last:
		Count
	};

	enum class SnapshotCommandType : quint8
	{
		Create,
		CreatePosNormal,
		Remove,
		ClearAll,
		// must be last:
		Count
	};

	enum class ObjectID : quint64
	{
		Dataset,
		SlicingPlane,
		Camera
	};

	template <typename T>
	T readVal(QDataStream& stream)
	{
		T t;
		stream >> t;
		if (stream.status() != QDataStream::Ok)
		{
			throw std::runtime_error(QString("Tried to read %1 bytes, but got status %2 instead of OK!").arg(sizeof(T)).arg(stream.status()).toStdString());
		}
		return t;
	}
	template <std::size_t N>
	void readArray(QDataStream& stream, std::array<float, N>& a)
	{
		int expectedBytes = static_cast<int>(sizeof(float) * N);
		auto actualBytes = stream.readRawData(reinterpret_cast<char*>(a.data()), expectedBytes);
		if (actualBytes != expectedBytes)
		{
			throw std::runtime_error(QString("readArray: Expected %1 but read %2 bytes!").arg(expectedBytes).arg(actualBytes).toStdString());
		}
	}
	template <std::size_t N>
	void writeArray(QDataStream& stream, std::array<float, N>const & a)
	{
		int expectedBytes = static_cast<int>(sizeof(float) * N);
		auto actualBytes = stream.writeRawData(reinterpret_cast<const char*>(a.data()), expectedBytes);
		if (actualBytes != expectedBytes)
		{
			throw std::runtime_error(QString("writeArray: Expected %1 but wrote %2 bytes!").arg(expectedBytes).arg(actualBytes).toStdString());
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
		//LOG(lvlInfo, QString("Plane: normal=(%2), quat=%3")
		//	.arg(arrayToString(vec2))
		//	.arg(arrayToString(quat))
		//);
		return quat;
	}

/*
	std::array<float, 3> quaternionToEulerAngles(std::array<float, 4> q)
	{
		double ayterm = 2 * (q[3] * q[1] - q[0] * q[2]);
		double a2[3] = {
			vtkMath::DegreesFromRadians(std::atan2(2 * (q[3] * q[0] + q[1] * q[2]), 1 - 2 * (q[0] * q[0] + q[1] * q[1]))),
			vtkMath::DegreesFromRadians(-vtkMath::Pi() / 2 + 2 * std::atan2(std::sqrt(1 + ayterm), std::sqrt(1 - ayterm))),
			vtkMath::DegreesFromRadians(std::atan2(2 * (q[3] * q[2] + q[0] * q[1]), 1 - 2 * (q[1] * q[1] + q[2] * q[2])))
		};
		for (int a = 0; a < 3; ++a)
		{   // round to nearest X degrees (for smoothing):
			const double RoundDegrees = 2;
			a2[a] = std::round(a2[a] / RoundDegrees) * RoundDegrees;
		}
		return a2;
	}

	std::array<float, 4> eulerAnglesToQuaternion(std::array<float, 3> a)
	{
		// from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
		float cr = std::cos(a[0] * 0.5);
		float sr = std::sin(a[0] * 0.5);
		float cp = std::cos(a[1] * 0.5);
		float sp = std::sin(a[1] * 0.5);
		float cy = std::cos(a[2] * 0.5);
		float sy = std::sin(a[2] * 0.5);
		return std::array<float, 4>
		{
			cr* cp* cy + sr * sp * sy,
				sr* cp* cy - cr * sp * sy,
				cr* sp* cy + sr * cp * sy,
				cr* cp* sy - sr * sp * cy
		};
	}
*/

	std::array<float, 3> DefaultPlaneNormal = { 0, 0, 1 };
//	std::array<float, 3> DefaultCameraViewDirection = { 0, 0, -1 };

	QString toHexStr(QByteArray const & ba)
	{
		QString result;
		for (auto b : ba)
		{
			result += QString("%1%2 ")
				.arg((b & 0xF0) >> 4, 0, 16)
				.arg((b & 0x0F), 0, 16);
		}
		return result;
		//
	}

	const quint64 NoSyncedClient = std::numeric_limits<quint64>::max();
	const quint64 ServerID = 1000; // ID of the server (for syncing its view/camera to clients)
	const quint64 ClientStartID = ServerID + 1;
}

class iAUnityWebsocketServerToolImpl : public QObject
{
private:
	template <typename T, std::size_t N>
	void unitToObjectPos(std::array<T, N>& pos) const
	{
		assert(N == 3);
		for (int i = 0; i < 3; ++i)
		{
			pos[i] *= m_maxSize;
		}
	}
	template <typename T, std::size_t N>
	void objectToUnitPos(std::array<T, N>& pos) const
	{
		assert(N == 3);
		for (int i = 0; i < 3; ++i)
		{
			pos[i] /= m_maxSize;
		}
	}

	template <std::size_t N>
	QByteArray msgObjectCommand(quint64 objID, ObjectCommandType objCmdType, std::array<float, N> const& values)
	{
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
		outStream << MessageType::Object << objCmdType << objID;
		writeArray<N>(outStream, values);
		return outData;
	}

	template <std::size_t N>
	void processObjectTransform(QDataStream& rcvStream, quint64 clientID, quint64 objID, ObjectCommandType objCmdType, iAMdiChild* child)
	{
		std::array<float, N> values{};
		readArray(rcvStream, values);
		LOG(lvlDebug, QString("    values=%1; broadcasting!").arg(arrayToString(values)));
		broadcastMsg(msgObjectCommand(objID, objCmdType, values), clientID);

		if (objID == 0)  // if info about object
		{
			if (clientID != m_syncedClientID)   // only apply to local objects if sync enabled
			{
				return;
			}
			auto renderer = child->dataSetViewer(m_dataSetID)->renderer();
			auto prop = renderer->vtkProp();
			std::array<double, N> dblVal;
			std::copy(values.begin(), values.end(), dblVal.begin());
			switch (objCmdType)
			{
			default:
				LOG(lvlWarn, QString("Received unsupported/not implemented command %1!").arg(enumToIntegral(objCmdType)));
				break;
			case ObjectCommandType::SetMatrix:
			{
				// TODO: TEST / Check whether used!
				assert(N == 16);
				vtkNew<vtkTransform> tr;
				tr->SetMatrix(dblVal.data());
				LOG(lvlDebug, QString("  Setting transformation matrix = (%1)").arg(arrayToString(dblVal)));
				prop->SetUserTransform(tr);
				break;
			}
			case ObjectCommandType::SetScaling:
			{
				// TODO: TEST / Check whether used!
				assert(N == 3);
				LOG(lvlDebug, QString("  Setting scale = (%1)").arg(arrayToString(dblVal)));
				prop->SetScale(dblVal.data());
				break;
			}
			case ObjectCommandType::SetTranslation:
			{
				// TODO: TEST / Check whether used!
				assert(N == 3);
				unitToObjectPos(dblVal);
				LOG(lvlDebug, QString("  Setting pos = (%1)").arg(arrayToString(dblVal)));
				prop->SetPosition(dblVal.data());
				break;
			}
			case ObjectCommandType::SetRotationEuler:
			{
				// TODO: TEST / Check whether used!
				LOG(lvlDebug, QString("  Setting euler rotation = (%1)").arg(arrayToString(dblVal)));
				prop->SetOrientation(dblVal.data());
				break;
			}
			case ObjectCommandType::SetRotationQuaternion:
			{
				assert(N == 4);
				// TODO: TEST / Check whether used!
				auto & q = values;
				double ayterm = 2 * (q[3] * q[1] - q[0] * q[2]);
				std::array<double, 3> angles = {
					vtkMath::DegreesFromRadians(
						std::atan2(2 * (q[3] * q[0] + q[1] * q[2]), 1 - 2 * (q[0] * q[0] + q[1] * q[1]))),
					vtkMath::DegreesFromRadians(
						-vtkMath::Pi() / 2 + 2 * std::atan2(std::sqrt(1 + ayterm), std::sqrt(1 - ayterm))),
					vtkMath::DegreesFromRadians(
						std::atan2(2 * (q[3] * q[2] + q[0] * q[1]), 1 - 2 * (q[1] * q[1] + q[2] * q[2])))};
				for (int a = 0; a < 3; ++a)
				{  // round to nearest X degrees:
					const double RoundDegrees = 2;
					angles[a] = std::round(angles[a] / RoundDegrees) * RoundDegrees;
				}
				LOG(lvlDebug, QString("  Setting rotatation: quat = (%1), angle = (%2)")
						.arg(arrayToString(q))
						.arg(arrayToString(angles)));
				auto bounds = renderer->bounds();
				auto center = (bounds.maxCorner() - bounds.minCorner()) / 2;
				double pos[3];
				prop->GetPosition(pos);
				vtkNew<vtkTransform> tr;
				tr->PostMultiply();
				tr->Translate(-center[0], -center[1], -center[2]);
				// rotation: order x-z-y, reverse direction of y
				tr->RotateX(angles[0]);
				tr->RotateZ(-angles[1]);
				tr->RotateY(angles[2]);
				// translation: y, z flipped; x, y reversed:
				tr->Translate(center[0] - pos[0], center[1] - pos[2], center[2] + pos[1]);
				prop->SetUserTransform(tr);
				break;
			}
			case ObjectCommandType::SetRotationNormalUp:
			{
				// TODO: TEST / Check whether used!
				LOG(lvlWarn, "  Setting rotation via normal and up NOT supported for object!");
				break;
			}
			}
		}
		/*
			// 
			else if (objID == 1) // ->Slicing plane
			{
				//	m_planeSliceTool->setMatrix(values); ???
			}
		*/
		else //if (objID == 2) // camera
		{
			if (objID != clientID)
			{
				LOG(lvlWarn, QString("Received invalid object (from client id = %1, for object id = %2) transform command: "
					"Client may only report changes to its own camera object!").arg(clientID).arg(objID));

			}
			// two options: either adapt 3D renderer directly (currently not implemented):
			//auto cam = child->renderer()->renderer()->GetActiveCamera();
			// or just visualize camera of other:
			auto vis = m_clientCamVis[clientID].get();
			std::array<double, N> dblVal;
			std::copy(values.begin(), values.end(), dblVal.begin());
			switch (objCmdType)
			{
			default:
				LOG(lvlWarn, QString("Received unsupported/not implemented command %1!").arg(enumToIntegral(objCmdType)));
				break;
			case ObjectCommandType::SetMatrix:
			{
				// TODO: TEST / Check whether used!
				assert(N == 16);
				vtkNew<vtkTransform> tr;
				tr->SetMatrix(dblVal.data());
				LOG(lvlInfo, QString("  Setting transformation matrix = (%1)").arg(arrayToString(dblVal)));
				//cam->SetUserTransform(tr);  // probably not working as expected - this is applied _in addition_ to other settings
					
				break;
			}
			case ObjectCommandType::SetScaling:
			{
				// TODO: TEST / Check whether used! -> only 1 value used
				assert(N == 3);
				LOG(lvlWarn, QString("  Setting scale = (%1) for viewer visualization NOT supported!").arg(arrayToString(dblVal)));
				//cam->SetParallelScale(dblVal[0]);  // probably not working as expected
				break;
			}
			case ObjectCommandType::SetTranslation:
			{
				// TODO: TEST / Check whether used!
				assert(N == 3);
				LOG(lvlInfo, QString("  Received pos = (%1)").arg(arrayToString(dblVal)));
				unitToObjectPos(dblVal);
				LOG(lvlInfo, QString("  Setting pos = (%1)").arg(arrayToString(dblVal)));
				//cam->SetPosition(dblVal.data());
				iAVec3d pos(dblVal.data());
				vis->update(pos, vis->dir(), vis->up());
				break;
			}
			case ObjectCommandType::SetRotationEuler:
			{
				// TODO: TEST / Check whether used!
				LOG(lvlWarn, "  Setting rotation for viewer visualization NOT supported!!");
				//cam->SetViewAngle(dblVal.data());
				// Todo: compute correct vector direction
				break;
			}
			case ObjectCommandType::SetRotationQuaternion:
			{
				LOG(lvlWarn, "  Setting rotation via quaternion for viewer visualization NOT supported!!");
				break;
			}
			case ObjectCommandType::SetRotationNormalUp:
			{
				iAVec3d dir{ -dblVal[0], -dblVal[1], -dblVal[2] };
				iAVec3d up { dblVal[3], dblVal[4], dblVal[5] };
				LOG(lvlInfo, QString("  Setting rotation, normal = (%1), up = (%2)")
					.arg(dir.toString()).arg(up.toString()));
				vis->update(vis->pos(), dir, up);
				break;
			}
			}
		}
		child->updateRenderer();
	}

	enum iAClientTableColumn
	{
		ID,
		Color,
		Address,
		Actions
	};

public:
	iAUnityWebsocketServerToolImpl(iAMainWindow* mainWnd, iAMdiChild* child) :
		m_wsServer(new QWebSocketServer(iAUnityWebsocketServerTool::Name, QWebSocketServer::NonSecureMode, this)),
		m_nextClientID(ClientStartID),
		m_dataState(DataState::NoDataset),
		m_clientListContainer(new QWidget(child)),
		m_clientTable(new QTableWidget(m_clientListContainer)),
		m_clientListDW(new iADockWidgetWrapper(m_clientListContainer, "Unity Clients", "ClientList", "https://github.com/3dct/open_iA/wiki/Remote")),
		m_syncedClientID(NoSyncedClient),
		m_child(child)
	{
		m_wsServer->moveToThread(&m_serverThread);
		connect(&m_serverThread, &QThread::finished, m_wsServer, &QWebSocketServer::close);
		m_serverThread.start();

		m_planeSliceTool = getTool<iAPlaneSliceTool>(child);
		if (!m_planeSliceTool)
		{
			m_planeSliceTool = addToolToActiveMdiChild<iAPlaneSliceTool>(iAPlaneSliceTool::Name, mainWnd, true);
		}
		using Self = iAUnityWebsocketServerToolImpl;
		connect(m_planeSliceTool, &iAPlaneSliceTool::snapshotAdded, this, &Self::addSnapshot);
		connect(m_planeSliceTool, &iAPlaneSliceTool::snapshotRemoved, this, &Self::removeSnapshot);
		connect(m_planeSliceTool, &iAPlaneSliceTool::snapshotsCleared, this, &Self::clearSnapshots);

		// attach to main renderer's camera, send message whenever it changes:
		auto cam = child->renderer()->renderer()->GetActiveCamera();
		cam->AddObserver(vtkCommand::ModifiedEvent, this, &Self::camModified);
		// potential improvement: send updates every x milliseconds only on modified, and send one final update on release?
		//child->renderer()->renderer()->AddObserver(vtkCommand::LeftButtonReleaseEvent, this, &Self::camModified);

		bool connected = connectWebSocket(m_wsServer, getValue(iARemotePortSettings::defaultAttributes(), iARemotePortSettings::UnityPort).toInt()) != 0;
		if (!connected)
		{
			LOG(lvlError, QString("%1: Setting up WebSocket server failed (error: %2)!")
				.arg(iAUnityWebsocketServerTool::Name)
				.arg(m_wsServer->errorString()));
			return;
		}
		auto listenMsg = QString("Listening on %1:%2").arg(m_wsServer->serverAddress().toString()).arg(m_wsServer->serverPort());
		LOG(lvlInfo, QString("%1: %2").arg(iAUnityWebsocketServerTool::Name).arg(listenMsg));
		child->splitDockWidget(child->renderDockWidget(), m_clientListDW, Qt::Vertical);
		m_clientListContainer->setLayout(new QVBoxLayout);
		m_clientListContainer->layout()->setContentsMargins(0, 0, 0, 0);
		m_clientListContainer->layout()->setSpacing(1);
		m_clientListContainer->layout()->addWidget(new QLabel(listenMsg));
		QStringList columnNames = QStringList() << "ID" << "Color" << "Client address" << "Actions";
		m_clientTable->setColumnCount(static_cast<int>(columnNames.size()));
		m_clientTable->setHorizontalHeaderLabels(columnNames);
		m_clientTable->verticalHeader()->hide();
		//m_clientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		m_clientTable->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
		m_clientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
		m_clientListContainer->layout()->addWidget(m_clientTable);

		m_dataSetID = child->firstImageDataSetIdx();
		auto bounds = child->dataSetViewer(m_dataSetID)->renderer()->bounds();
		m_maxSize = std::max({
			bounds.maxCorner().x() - bounds.minCorner().x(),
			bounds.maxCorner().y() - bounds.minCorner().y(),
			bounds.maxCorner().z() - bounds.minCorner().z(),
		});
		dynamic_cast<iAImageData*>(child->dataSet(m_dataSetID).get())->vtkImage()->GetSpacing(m_spacing.data());
		connect(m_wsServer, &QWebSocketServer::newConnection, this, [this, child]
		{
			auto client = m_wsServer->nextPendingConnection();
			LOG(lvlInfo, QString("%1: Client connected: %2:%3")
				.arg(iAUnityWebsocketServerTool::Name)
				.arg(client->peerAddress().toString())
				.arg(client->peerPort()));
			auto clientID = m_nextClientID++;  // simplest possible ID assignment: next free ID. Maybe random?

			int clientRow = m_clientTable->rowCount();
			m_clientTable->insertRow(clientRow);
			auto idItem = new QTableWidgetItem(QString::number(clientID));
			idItem->setData(Qt::UserRole, clientID);
			m_clientTable->setItem(clientRow, iAClientTableColumn::ID, idItem);

			auto theme = iAColorThemeManager::theme("Brewer Paired (max. 12)");
			auto colorIdx = (2 * (clientID - ClientStartID)) % theme->size();
			auto clientColorBody = theme->color(colorIdx);
			auto clientColorVec = theme->color((colorIdx + 1) % theme->size());
			auto colorWidget = new QLabel("Test", m_clientTable);
			//colorWidget->setGeometry(0, 0, 16, 16);
			colorWidget->setMinimumSize(16, 16);
			colorWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			colorWidget->setAutoFillBackground(true);
			colorWidget->setStyleSheet(QString("background-color: %1").arg(clientColorBody.name()));
			m_clientTable->setCellWidget(clientRow, iAClientTableColumn::Color, colorWidget);

			m_clientTable->setItem(clientRow, iAClientTableColumn::Address, new QTableWidgetItem(client->peerAddress().toString()));

			auto actionWidget = new QWidget();
			actionWidget->setLayout(new QHBoxLayout);
			actionWidget->layout()->setContentsMargins(0, 0, 0, 0);
			actionWidget->layout()->setSpacing(1);

			auto syncAction = new QAction("Sync");
			syncAction->setToolTip("Synchronize Views between this client and this window.");
			syncAction->setCheckable(true);
			syncAction->setChecked(false);
			m_syncActions.insert(std::make_pair(clientID, syncAction));
			QObject::connect(syncAction, &QAction::toggled, m_clientTable, [this, clientID, syncAction]()
			{
				bool checked = syncAction->isChecked();
				m_syncedClientID = (checked) ? clientID : NoSyncedClient;
				for (auto const & s : m_syncActions)
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
			auto syncButton = new QToolButton(actionWidget);
			syncButton->setDefaultAction(syncAction);

			auto disconnectAction = new QAction("Sync");
			disconnectAction->setToolTip("Synchronize Views between this client and this window.");
			disconnectAction->setCheckable(true);
			disconnectAction->setChecked(false);
			QObject::connect(disconnectAction, &QAction::triggered, m_clientTable, [this, clientID]()
			{
				m_clientSocket[clientID]->close(QWebSocketProtocol::CloseCodeNormal, "Server user manually requested client disconnect.");
			});
			iAMainWindow::get()->addActionIcon(disconnectAction, "close");
			auto disconnectButton = new QToolButton(actionWidget);
			disconnectButton->setDefaultAction(disconnectAction);

			actionWidget->layout()->addWidget(syncButton);
			actionWidget->layout()->addWidget(disconnectButton);
			m_clientTable->setCellWidget(clientRow, iAClientTableColumn::Actions, actionWidget);

			m_clientSocket[clientID] = client;
			m_clientState[clientID] = ClientState::AwaitingProtocolNegotiation;

			m_clientCamVis[clientID] = std::make_unique<iACameraVis>(child->renderer()->renderer(), m_maxSize / 20, clientColorBody, clientColorVec);
			connect(m_clientCamVis[clientID].get(), &iACameraVis::updateRequired, this, [this, clientID, child]
			{
				m_clientCamVis[clientID]->updateSource();
				child->updateRenderer();
			});
			m_clientCamVis[clientID]->show();

			connect(client, &QWebSocket::stateChanged, this, [clientID](QAbstractSocket::SocketState state)
			{
				LOG(lvlDebug, QString("%1: Client (ID=%2): socket state changed to %3")
					.arg(iAUnityWebsocketServerTool::Name).arg(clientID)
					.arg(QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(state)));
			});
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
			connect(client, &QWebSocket::errorOccurred, this, [clientID](QAbstractSocket::SocketError error)
			{
				LOG(lvlDebug, QString("%1: Client (ID=%2): error occurred: %3")
					.arg(iAUnityWebsocketServerTool::Name).arg(clientID)
					.arg(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(error)));
			});
#endif
			connect(client, &QWebSocket::pong, this, [clientID](quint64 elapsedTime, const QByteArray& payload)
			{
				Q_UNUSED(payload);
				LOG(lvlDebug, QString("%1: Client (ID=%2): pong received; elapsed: %3 ms.")
					.arg(iAUnityWebsocketServerTool::Name).arg(clientID)
					.arg(elapsedTime));
			});
			connect(client, &QWebSocket::textMessageReceived, this, [clientID](QString message)
			{
				LOG(lvlWarn, QString("%1: Client (ID=%2): TEXT MESSAGE received: %3!")
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
				handleBinaryMessage(rcvData, clientID, child);
			});
			connect(client, &QWebSocket::disconnected, this, [this, client, clientID]
			{
				//QWebSocket* client = qobject_cast<QWebSocket*>(sender());
				LOG(lvlInfo, QString("%1: Client (ID=%2, %3:%4) disconnected!")
					.arg(iAUnityWebsocketServerTool::Name)
					.arg(clientID)
					.arg(client->peerAddress().toString())
					.arg(client->peerPort()));
				m_clientSocket.erase(clientID);
				m_clientState.erase(clientID);
				m_syncActions.erase(clientID);
				m_clientCamVis.erase(clientID); // destructor automatically hides visualization
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

	void stop()
	{
		auto serverPort = m_wsServer->serverPort();
		// m_wsServer->close(); is called through QThread::finish signal, to make sure it is called in the server's thread
		m_serverThread.quit();
		m_serverThread.wait();
		removeClosedPort(serverPort);
		LOG(lvlInfo, QString("%1 STOP").arg(iAUnityWebsocketServerTool::Name));
	}

private:
	void sendMessage(quint64 clientID, QByteArray const& b)
	{
		//LOG(lvlDebug, QString("Sending message to client %1; data: %2").arg(clientID).arg(toHexStr(b)));
		m_clientSocket[clientID]->sendBinaryMessage(b);
	}

	void sendMessage(quint64 clientID, MessageType t)
	{
		// if (t == MessageType::ACK || t == MessageType::NAK)
		QByteArray b;
		b.append(enumToIntegral(t));
		sendMessage(clientID, b);
	}

	void sendClientID(quint64 clientID)
	{
		QByteArray b;
		QDataStream stream(&b, QIODevice::WriteOnly);
		stream << enumToIntegral(MessageType::ClientID);
		stream << clientID;  // QDataStream does Big Endian conversions automatically
		sendMessage(clientID, b);
	}

	void sendSnapshots(quint64 clientID)
	{
		auto cs = m_clientSocket[clientID];
		for (auto s : m_planeSliceTool->snapshots())
		{
			cs->sendBinaryMessage(msgSnapshotAdd(s.first, s.second));
		}
	}

	void broadcastMsg(QByteArray const& b, quint64 exceptClientID = std::numeric_limits<quint64>::max())
	{
		for (auto c : m_clientSocket)
		{
			if (c.first == exceptClientID)
			{
				continue;
			}
			sendMessage(c.first, b);
		}
	}

	QByteArray msgSnapshotAdd(quint64 snapshotID, iASnapshotInfo const& info) const
	{
		QByteArray outData;
		QDataStream stream(&outData, QIODevice::WriteOnly);
		stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
		stream << MessageType::Snapshot << SnapshotCommandType::CreatePosNormal << snapshotID;
		std::array<float, 3> clientPos(info.position);
		objectToUnitPos(clientPos);
		writeArray(stream, clientPos);
		//auto quat = getRotationQuaternionFromVectors(DefaultPlaneNormal, info.normal);
		//writeArray(stream, quat);
		writeArray(stream, info.normal);
		return outData;
	}

	void addSnapshot(quint64 snapshotID, iASnapshotInfo const& info)
	{
		LOG(lvlInfo, QString("  New snapshot, ID=%1; position=%2, normal=%3")
				.arg(snapshotID)
				.arg(arrayToString(info.position))
				.arg(arrayToString(info.normal)));
		broadcastMsg(msgSnapshotAdd(snapshotID, info));
	}

	void removeSnapshot(quint64 snapshotID)
	{
		LOG(lvlInfo, QString("  Removing snapshot with ID=%1.").arg(snapshotID));
		QByteArray outData;
		QDataStream stream(&outData, QIODevice::WriteOnly);
		stream << MessageType::Snapshot << SnapshotCommandType::Remove << snapshotID;
		broadcastMsg(outData);
	}

	QByteArray msgSnapshotsClear() const
	{
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream << MessageType::Snapshot << SnapshotCommandType::ClearAll;
		return outData;
	}

	void clearSnapshots()
	{
		LOG(lvlInfo, QString("  Clearing all snapshots."));
		broadcastMsg(msgSnapshotsClear());
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
		auto childFileName = child->dataSet(m_dataSetID)->metaData(iADataSet::FileNameKey).toString();
		return fileName == childFileName || fileName == QFileInfo(childFileName).fileName();
	}

	QByteArray msgDatasetLoad()
	{
		QByteArray fnBytes = m_dataSetFileName.toUtf8();
		quint32 fnLen = static_cast<quint32>(fnBytes.size());
		QByteArray outData;
		QDataStream outStream(&outData, QIODevice::WriteOnly);
		outStream << MessageType::Command << CommandType::LoadDataset << fnLen;
		auto bytesWritten = outStream.writeRawData(fnBytes, fnLen);
		if (bytesWritten != static_cast<int>(fnLen))
		{
			LOG(lvlWarn, QString("data loading: Expected %1 but wrote %2 bytes!").arg(bytesWritten).arg(fnLen));
		}
		return outData;
	}

	void sendDataLoadingRequest(QString const& fileName)
	{
		LOG(lvlInfo, QString("  Broadcasting data loading request to all clients; filename: %1").arg(fileName));
		m_dataState = DataState::PendingClientAck;
		m_dataSetFileName = fileName;
		broadcastMsg(msgDatasetLoad());
		for (auto const & c : m_clientSocket)
		{
			m_clientState[c.first] = ClientState::PendingDatasetAck;
		}
	}

	std::array<float, 16> objectMatrix(quint64 objID, iAMdiChild* child) const
	{
		std::array<float, 16> matrix;
		if (objID == 0)
		{
			auto prop = child->dataSetViewer(child->firstImageDataSetIdx())->renderer()->vtkProp();
			std::array<double, 16> matrixDouble;
			prop->GetMatrix(matrixDouble.data());
			// TODO: scale to unit positions?
			std::copy(matrixDouble.begin(), matrixDouble.end(), matrix.begin());
		}
		else
		{
			vtkNew<vtkMatrix4x4> m;m->Identity();
			std::copy(m->GetData(), m->GetData() + 16, matrix.begin());
		}

		return matrix;
	}

	void resyncClient(quint64 clientID, iAMdiChild* child)
	{
		auto cs = m_clientSocket[clientID];
		// send all object matrices:
		for (quint64 o : {0, 1})
		{
			cs->sendBinaryMessage(msgObjectCommand(o, ObjectCommandType::SetMatrix, objectMatrix(o, child)));
		}

		// clear and resend all snapshots:
		cs->sendBinaryMessage(msgSnapshotsClear());
		sendSnapshots(clientID);
	}

	void handleClientDataResponse(quint64 clientID, MessageType type, iAMdiChild* child)
	{
		if (type == MessageType::NAK)
		{
			LOG(lvlInfo, QString("  Received NAK from client %1, aborting data loading by broadcasting NAK.").arg(clientID));
			// broadcast NAK:
			for (auto const & s : m_clientSocket)
			{
				sendMessage(s.first, MessageType::NAK);
				m_clientState[s.first] = ClientState::Idle;
			}
			m_dataState = DataState::NoDataset;  // maybe switch back to previous dataset if there is any?
			m_dataSetFileName = "";
			for (auto outOfSyncClientID : m_outOfSync)
			{
				resyncClient(outOfSyncClientID, child);
			}
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

	void handleBinaryMessage(QByteArray const & rcvData, quint64 clientID, iAMdiChild* child)
	{
		try
		{
			QDataStream rcvStream(rcvData);
			rcvStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
			MessageType type;
			rcvStream >> type;
			/*
			LOG(lvlDebug, QString("%1: Received message of type %2 from client ID=%3; (data: %4)")
					.arg(iAUnityWebsocketServerTool::Name)
					.arg(static_cast<int>(type))
					.arg(clientID)
					.arg(toHexStr(rcvData)));
			*/
			if (m_clientState[clientID] == ClientState::AwaitingProtocolNegotiation)
			{
				handleVersionNegotiation(type, clientID, rcvStream);  // no need to handle return type (yet)
				return;
			}
			// when waiting on feedback for load dataset message, only (above) protocol negotiation for new clients is permitted
			if (m_dataState == DataState::PendingClientAck)
			{
				if (type != MessageType::NAK && type != MessageType::ACK)
				{
					LOG(lvlError, QString("  Invalid message from client %1: We are currently waiting for client response on dataset loading: "
								"Only ACK or NAK are permissible; ignoring message!")
							.arg(clientID));
					m_outOfSync.insert(clientID);    // add to clients currently out of sync (-> send resync if dataset loading fails)
					return;
				}
				handleClientDataResponse(clientID, type, child);
				return;
			}
			if (m_clientState[clientID] == ClientState::PendingSingleClientDatasetAck)
			{
				if (type != MessageType::ACK)
				{
					LOG(lvlError, QString("  Client %1: While waiting for client ACK of already loaded dataset, received message type %2. "
						"Client either does not have the dataset or sent an invalid message, disconnecting to avoid de-synchronized state!")
						.arg(clientID).arg(static_cast<int>(type)));
					m_clientSocket[clientID]->close(QWebSocketProtocol::CloseCodeProtocolError,
						"Waited for client ACK of dataset - received NAK or other message, so no synchronized state possible.");
					return;
				}
				else    // client could load dataset! let's sync its state:
				{
					m_clientState[clientID] = ClientState::Idle;
					resyncClient(clientID, child);
				}
			}

			if (rcvData.size() < 2)
			{
				LOG(lvlError, QString("  Invalid command: missing subcommand; ignoring message!"));
				return;
			}
			if (m_clientState[clientID] != ClientState::Idle)
			{
				LOG(lvlWarn, "Received unexpected message while client not in Idle state!");
			}
			switch (type)
			{
			case MessageType::Command:
			{
				if (!isInEnum<CommandType>(rcvData[1]))
				{  // encountered value and valid range output already in isInEnum!
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
				else  // CommandType::LoadDataset:
				{
					LOG(lvlInfo, QString("  Load Dataset received"));
					// https://forum.qt.io/topic/89832/reading-char-array-from-qdatastream
					auto fnLen = readVal<quint32>(rcvStream);
					QByteArray fnBytes;
					fnBytes.resize(fnLen);
					auto readBytes = rcvStream.readRawData(fnBytes.data(), fnLen);
					if (static_cast<quint32>(readBytes) != fnLen)
					{
						LOG(lvlError,
							QString("    Invalid message: expected length %1, actually read %2 bytes")
								.arg(fnLen)
								.arg(readBytes));
						return;
					}
					QString fileName = QString::fromUtf8(fnBytes);
					LOG(lvlInfo,
						QString("  Client requested loading dataset from filename '%1' (len=%2).")
							.arg(fileName)
							.arg(fnLen));

					if (!dataSetExists(fileName, child))
					{
						LOG(lvlWarn, "  Requested dataset does not exist, sending NAK!");
						sendMessage(clientID, MessageType::NAK);
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
				auto objID = readVal<quint64>(rcvStream);
				LOG(lvlDebug, QString("  Object subcommand %1 for object ID %2").arg(static_cast<int>(objCommand)).arg(objID));
				switch (objCommand)
				{
				case ObjectCommandType::SetMatrix:
					processObjectTransform<16>(rcvStream, clientID, objID, objCommand, child);
					break;
				case ObjectCommandType::SetTranslation:
					processObjectTransform<3>(rcvStream, clientID, objID, objCommand, child);
					break;
				case ObjectCommandType::SetScaling:
					processObjectTransform<3>(rcvStream, clientID, objID, objCommand, child);
					break;
				case ObjectCommandType::SetRotationQuaternion:
					processObjectTransform<4>(rcvStream, clientID, objID, objCommand, child);
					break;
				case ObjectCommandType::SetRotationEuler:
				{
					auto axis = readVal<quint8>(rcvStream);
					float value = readVal<float>(rcvStream);
					LOG(lvlDebug,
						QString("  Object command=SetRotation (Euler Angles) received for object ID=%1 with axis=%3, "
								"value=%4.")
							.arg(static_cast<int>(objCommand))
							.arg(objID)
							.arg(axis)
							.arg(value));
					QByteArray outData;
					QDataStream outStream(&outData, QIODevice::WriteOnly);
					outStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
					outStream << MessageType::Object << objCommand << objID << axis << value;
					broadcastMsg(outData, clientID);
					// TODO: local application!
					break;
				}
				case ObjectCommandType::SetRotationNormalUp:
					processObjectTransform<6>(rcvStream, clientID, objID, objCommand, child);
					break;
				default:
					LOG(lvlWarn, QString("  Object subcommand %1 not implemented!").arg(static_cast<int>(objCommand)));
					break;
				}
				break;
			}
			case MessageType::Snapshot:
			{
				if (!isInEnum<SnapshotCommandType>(rcvData[1]))
				{  // encountered value and valid range output already in isInEnum!
					LOG(lvlError,
						QString("  Invalid snapshot message: subcommand not in valid range; ignoring message!"));
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
					unitToObjectPos(info.position);
					std::array<float, 4> rotation;
					readArray(rcvStream, rotation);
					LOG(lvlWarn, QString("  New Snapshot: position %1, rotation %2: Note that rotation quaternion conversion is EXPERIMENTAL!")
						.arg(arrayToString(info.position)).arg(arrayToString(rotation)));
					info.normal = applyRotationToVector(DefaultPlaneNormal, rotation);
					auto snapshotIDRow = m_planeSliceTool->addSnapshot(info);
					addSnapshot(snapshotIDRow.first, info);
					break;
				}
				case SnapshotCommandType::CreatePosNormal:
				{
					iASnapshotInfo info{};
					readArray(rcvStream, info.position);
					unitToObjectPos(info.position);
					readArray(rcvStream, info.normal);
					LOG(lvlInfo, QString("  New Snapshot: position %1, normal %2")
						.arg(arrayToString(info.position)).arg(arrayToString(info.normal)));
					auto snapshotIDRow = m_planeSliceTool->addSnapshot(info);
					addSnapshot(snapshotIDRow.first, info);
					break;
				}
				case SnapshotCommandType::Remove:
				{
					auto snapshotID = readVal<quint64>(rcvStream);
					m_planeSliceTool->removeSnapshot(snapshotID);
					removeSnapshot(snapshotID);
					break;
				}
				case SnapshotCommandType::ClearAll:
					m_planeSliceTool->clearSnapshots();
					clearSnapshots();
					break;
				default:
					LOG(lvlWarn, "Unsupported Snapshot command!");
					break;
				}
				break;
			}
			default:
				LOG(lvlWarn, "Unsupported Snapshot command!");
				break;
			}
		}
		catch (std::exception& e)
		{
			LOG(lvlError, QString("%1: Error: %2").arg(iAUnityWebsocketServerTool::Name).arg(e.what()));
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
			for (auto const & s : m_clientSocket)
			{
				sendMessage(s.first, MessageType::ACK);
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
		auto clientProtocolVersion = readVal<quint64>(stream);
		// TODO: check whether we have read enough bytes!
		if (clientProtocolVersion > ServerProtocolVersion)
		{
			LOG(lvlWarn, QString("Client advertised unsupported protocol version %1 (we only support up to version %2), sending NAK...")
				.arg(clientProtocolVersion)
				.arg(ServerProtocolVersion));
			sendMessage(clientID, MessageType::NAK);
		}
		else
		{
			LOG(lvlInfo, QString("Client advertised supported protocol version %1, sending ACK...")
				.arg(clientProtocolVersion));
			sendMessage(clientID, MessageType::ACK);
			sendClientID(clientID);
			sendSnapshots(clientID);
			if (m_dataState == DataState::NoDataset)
			{
				m_clientState[clientID] = ClientState::Idle;
			}
			else
			{
				m_clientSocket[clientID]->sendBinaryMessage(msgDatasetLoad());
				m_clientState[clientID] = (m_dataState == DataState::PendingClientAck) ?
					ClientState::PendingDatasetAck :      // dataset loading still in progress; wait for this client's response as well
					ClientState::PendingSingleClientDatasetAck; // special state: only wait for this client's response, and disconnect it if he sends NAK
			}
		}
	}

	QWebSocketServer* m_wsServer;
	std::map<quint64, QWebSocket*> m_clientSocket;
	std::map<quint64, ClientState> m_clientState;
	std::map<quint64, QAction*> m_syncActions;
	std::map<quint64, std::unique_ptr<iACameraVis>> m_clientCamVis;
	std::set<quint64> m_outOfSync;
	quint64 m_nextClientID;
	QThread m_serverThread;
	//! dataset state: currently a dataset is (1) not loaded (2) being loaded (3) loaded and sent out to clients
	DataState m_dataState;
	QString m_dataSetFileName;
	iAPlaneSliceTool* m_planeSliceTool;

	QWidget* m_clientListContainer;
	QTableWidget* m_clientTable;
	iADockWidgetWrapper* m_clientListDW;
	quint64 m_syncedClientID;
	iAMdiChild* m_child;
	double m_maxSize;
	size_t m_dataSetID;
	std::array<double, 3> m_spacing;
	iAVec3d m_camLastPos, m_camLastDir;

private slots:
	void camModified()
	{
		auto cam = m_child->renderer()->renderer()->GetActiveCamera();
		iAVec3d posDbl, dirDbl;
		cam->GetPosition(posDbl.data());
		cam->GetDirectionOfProjection(dirDbl.data());
		const double CamChangePosEpsilon = m_maxSize * 0.01;
		const double CamChangeRotEpsilon = 0.01;
		if ( (posDbl - m_camLastPos).length() > CamChangePosEpsilon)
		{
			m_camLastPos = posDbl;
			std::array<float, 3> posFlt{ static_cast<float>(posDbl[0]), static_cast<float>(posDbl[1]), static_cast<float>(posDbl[2]) };
			objectToUnitPos(posFlt);
			//LOG(lvlDebug, QString("Camera position modified; new: %1 (unit pos: %2)").arg(posDbl.toString()).arg(arrayToString(posFlt)));
			broadcastMsg(msgObjectCommand(ServerID, ObjectCommandType::SetTranslation, posFlt));
		}
		if ((dirDbl.normalized() - m_camLastDir).length() > CamChangeRotEpsilon)
		{
			m_camLastDir = dirDbl.normalized();
			//LOG(lvlDebug, QString("Camera rotation modified; new: %1").arg(dirDbl.toString()));
			//auto quat = getRotationQuaternionFromVectors(DefaultCameraViewDirection, dirFlt);
			//broadcastMsg(msgObjectCommand(ServerID, ObjectCommandType::SetRotationQuaternion, quat));
			std::array<double, 3> upDbl;
			cam->GetViewUp(upDbl.data());
			std::array<float, 6> data = {
				static_cast<float>(dirDbl[0]), static_cast<float>(dirDbl[1]), static_cast<float>(dirDbl[2]),
				static_cast<float>(upDbl[0]), static_cast<float>(upDbl[1]), static_cast<float>(upDbl[2])
			};
			broadcastMsg(msgObjectCommand(ServerID, ObjectCommandType::SetRotationNormalUp, data));
		}
	}
};

const QString iAUnityWebsocketServerTool::Name("Unity Volume Interaction Server");

iAUnityWebsocketServerTool::iAUnityWebsocketServerTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_impl(std::make_unique<iAUnityWebsocketServerToolImpl>(mainWnd, child))
{
	if (child->firstImageDataSetIdx() == iAMdiChild::NoDataSet || !child->dataSetViewer(child->firstImageDataSetIdx()))
	{
		throw std::runtime_error("Unity volume interaction server: Volume/Image dataset required but none available; either no such dataset is loaded, or it is not fully initialized yet!");
	}
}

iAUnityWebsocketServerTool::~iAUnityWebsocketServerTool()
{
	m_impl->stop();
}
