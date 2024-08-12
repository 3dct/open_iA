// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAWebsocketAPI.h"

#include "iAJPGImage.h"
#include "iARemoteAction.h"

#include <iALog.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocketServer>
#include <QWebSocket>

namespace
{
	QList<iAWSClient>::iterator findClient(QList<iAWSClient>& list, QWebSocket* client)
	{
		auto it = std::find_if(list.begin(), list.end(),
			[client](iAWSClient const& clientData) { return clientData.ws == client; });
		assert(it != list.end());
		return it;
	}
}

iAWebsocketAPI::iAWebsocketAPI(quint16 port):
	m_port(port),
	m_count(0), // possibly use a separate counter per client?
	m_clientID(0)
{
}

void iAWebsocketAPI::init()
{
	m_wsServer = new QWebSocketServer(QStringLiteral("Remote Server"), QWebSocketServer::NonSecureMode, this);
	if (m_wsServer->listen(QHostAddress::Any, m_port))
	{
		connect(m_wsServer, &QWebSocketServer::newConnection, this, &iAWebsocketAPI::onNewConnection);
		connect(m_wsServer, &QWebSocketServer::closed, this, &iAWebsocketAPI::closed);
	}
	std::vector<iAAnnotation> captions;
	updateCaptionList(captions);
}

bool iAWebsocketAPI::setRenderedImage(std::shared_ptr<iAJPGImage> img, QString viewID)
{
	// TODO: check other thread communication methods, e.g. locking images via mutex; currently using signal/slot
	if (images.contains(viewID) && images[viewID]->data == img->data)
	{
		LOG(lvlDebug, "Setting same image again, ignoring!");
		return false;
	}
	images.insert(viewID, img);
	return true;
}

void iAWebsocketAPI::close()
{
	//qDeleteAll(m_clients);	// any client QWebSockets are closed and deleted (see ~QWebSocketServer documentation)
	m_wsServer->close();
}

void iAWebsocketAPI::onNewConnection()
{
	QWebSocket* client = m_wsServer->nextPendingConnection();
	LOG(lvlDebug, QString("Client connected: %1:%2").arg(client->peerAddress().toString()).arg(client->peerPort()));
	connect(client, &QWebSocket::textMessageReceived, this, &iAWebsocketAPI::processTextMessage);
	//connect(client, &QWebSocket::binaryMessageReceived, this, &iAWebsocketAPI::processBinaryMessage);    // clients don't send binary messages at the moment
	connect(client, &QWebSocket::disconnected, this, &iAWebsocketAPI::socketDisconnected);
	iAWSClient ws{ client, m_clientID++, 0, 0 };
	m_clients << ws;
	emit clientConnected(ws.id);
}

void iAWebsocketAPI::processTextMessage(QString message)
{
	m_StoppWatch.restart();
	QWebSocket*  client = qobject_cast<QWebSocket*>(sender());
	auto it = findClient(m_clients, client);
	it->rcvd += message.toUtf8().size();
	emit clientTransferUpdated(it->id, it->rcvd, it->sent);

	auto request = QJsonDocument::fromJson(message.toLatin1());
	if (request["method"].toString() == "wslink.hello")
	{
		commandWslinkHello(request, client);
	}
	else if (request["method"].toString() == "viewport.image.push.observer.add")
	{
		commandAddObserver(request, client);
	}
	else if (request["method"].toString() == "viewport.image.push")
	{
		commandImagePush(request, client);
	}
	else if (request["method"].toString() == "viewport.image.push.original.size")
	{
		commandImagePushSize(request, client);
	}
	else if (request["method"].toString() == "viewport.image.push.invalidate.cache")
	{
		commandImagePushInvalidateCache(request, client);
	}
	else if (request["method"].toString() == "viewport.image.push.quality")
	{
		commandImagePushQuality(request, client);
	}
	else if (request["method"].toString() == "viewport.mouse.interaction")
	{
		commandControls(request, client);
	}
	else if (request["method"].toString() == "viewport.mouse.zoom.wheel")
	{
		commandControls(request, client);
	}

	//Captions API
	else if (request["method"].toString() == "subscribe.captions")
	{
		captionSubscribe(client);
	}
	else if (request["method"].toString() == "select.caption")
	{
		emit selectCaption(request["id"].toInt());
	}
	else if (request["method"].toString() == "remove.caption")
	{
		emit removeCaption(request["id"].toInt());
	}
	else if (request["method"].toString() == "addMode.caption")
	{
		emit addMode();
	}
	else if (request["method"].toString() == "nameChanged.caption")
	{
		emit changeCaptionTitle(request["id"].toInt(), request["title"].toString());
	}
	else if (request["method"].toString() == "hideAnnotation.caption")
	{
		emit hideCaption(request["id"].toInt());
	}
}

void iAWebsocketAPI::sendTextMessage(QByteArray const & data, QWebSocket* client)
{
	auto it = findClient(m_clients, client);
	it->sent += data.size();
	client->sendTextMessage(data);
	emit clientTransferUpdated(it->id, it->rcvd, it->sent);
}

void iAWebsocketAPI::sendBinaryMessage(QByteArray const& data, QWebSocket* client)
{
	auto it = findClient(m_clients, client);
	it->sent += data.size();
	client->sendBinaryMessage(data);
	emit clientTransferUpdated(it->id, it->rcvd, it->sent);
}

void iAWebsocketAPI::commandWslinkHello(QJsonDocument request, QWebSocket* client)
{
	const auto clientID = QJsonObject{{"clientID", QUuid::createUuid().toString()}};
	const auto resultArray = QJsonArray{clientID};
	auto wsLinkHelloResponse = QJsonObject{
		{"wslink", "1.0" },
		{"id", request["id"].toString()},
		{"result", clientID}
	};
	sendTextMessage(QJsonDocument{ wsLinkHelloResponse }.toJson(), client);
}

void iAWebsocketAPI::commandAddObserver(QJsonDocument request, QWebSocket* client)
{
	QString viewIDString = request["args"][0].toString();
	const auto viewIDResponse = QJsonObject{
		{"result", "success"},
		{"viewId", viewIDString}
	};
	auto addObserverResponse = QJsonObject{
		{"wslink", "1.0"},
		{"result", viewIDResponse}
	};
	sendTextMessage(QJsonDocument{ addObserverResponse }.toJson(), client);

	if (subscriptions.contains(viewIDString))
	{
		subscriptions[viewIDString].append(client);
	}
	else
	{
		subscriptions.insert(viewIDString,  QList<QWebSocket*>());
		subscriptions[viewIDString].append(client);
	}
}

void iAWebsocketAPI::commandImagePush(QJsonDocument request, QWebSocket* client)
{
	sendSuccess(request, client);
	QString viewIDString = request["args"][0]["view"].toString();
	sendImage(client, viewIDString);
}

void iAWebsocketAPI::commandImagePushSize(QJsonDocument request, QWebSocket* client)
{
	sendSuccess(request, client);
}

void iAWebsocketAPI::commandImagePushInvalidateCache(QJsonDocument request, QWebSocket* client)
{
	sendSuccess(request, client);
}

void iAWebsocketAPI::commandImagePushQuality(QJsonDocument request, QWebSocket* client)
{
	sendSuccess(request, client);
}

void iAWebsocketAPI::sendSuccess(QJsonDocument request, QWebSocket* client)
{
	Q_UNUSED(request);
	auto successResponseObj = QJsonObject{
		{"wslink", "1.0"},
		{"result", QJsonObject{{"result", "success"}}}
	};
	sendTextMessage(QJsonDocument{ successResponseObj }.toJson(), client);
}

void iAWebsocketAPI::commandControls(QJsonDocument request, QWebSocket* client)
{
	iARemoteAction * webAction = new iARemoteAction();
	auto argList = request["args"][0];

	if (argList["action"] == "down")
	{
		webAction->action = iARemoteAction::ButtonDown;
	}
	else if (argList["action"] == "up")
	{
		webAction->action = iARemoteAction::ButtonUp;
	}
	else if (argList["type"] == "MouseWheel")
	{
		webAction->action = iARemoteAction::MouseWheel;
	}
	else if (argList["type"] == "EndMouseWheel")
	{
		webAction->action = iARemoteAction::EndMouseWheel;
	}
	else if (argList["type"] == "StartMouseWheel")
	{
		webAction->action = iARemoteAction::StartMouseWheel;
	}
	else
	{
		webAction->action = iARemoteAction::Unknown;
	}

	webAction->buttonLeft = argList["buttonLeft"].toInt();
	webAction->buttonRight = argList["buttonRight"].toInt();
	webAction->buttonMiddle = argList["buttonMiddle"].toInt();
	webAction->altKey = argList["altKey"].toInt();
	webAction->ctrlKey = argList["controlKey"].toBool() || argList["ctrlKey"].toInt();
	webAction->metaKey = argList["metaKey"].toInt();
	webAction->shiftKey = argList["shiftKey"].toInt() || argList["shiftKey"].toBool();
	webAction->viewID = argList["view"].toString();
	webAction->x = argList["x"].toDouble();
	webAction->y = argList["y"].toDouble();
	webAction->spinY = argList["spinY"].toDouble();
	{
		// TODO: potential for speedup: avoid dynamic allocation here!
		// enqueue in local data structure to enable receiver to apply all actions at once:
		std::lock_guard<std::mutex> guard(m_actionsMutex);
		m_actions.push_back(webAction);
	}
	// notify GUI that control message has come in:
	emit controlCommand();
	sendSuccess(request, client);
}

QList<iARemoteAction*> iAWebsocketAPI::getQueuedActions()
{
	std::lock_guard<std::mutex> guard(m_actionsMutex);
	QList<iARemoteAction*> result;
	for (auto a : m_actions)
	{
		result.push_back(a);
	}
	m_actions.clear();
	return result;
}

QString iAWebsocketAPI::listenAddress() const
{
	return QString("%1:%2").arg(m_wsServer->serverAddress().toString()).arg(m_wsServer->serverPort());
}

void iAWebsocketAPI::sendImage(QWebSocket* client, QString viewID)
{
	auto imgIDString = QString("wslink_bin%1").arg(m_count);
	auto imgHeaderObj = QJsonObject{
		{"wslink", "1.0"},
		{"method", "wslink.binary.attachment"},
		{"args", QJsonArray{ imgIDString } }
	};
	sendTextMessage(QJsonDocument{ imgHeaderObj }.toJson(), client);

	QByteArray const & ba = images[viewID]->data;
	int width  = images[viewID]->width;
	int height = images[viewID]->height;
	sendBinaryMessage(ba, client);

	auto imageSize = ba.size();
	const auto imgDescriptorObj = QJsonObject{
		{"format", "jpeg"},
		{"global_id", 1},
		{"global_id", "1"},         // check: duplicate required??
		{"id", viewID},
		{"image", imgIDString},
		{"localTime", 0},
		{"memsize", imageSize},
		{"mtime", 2125+m_count*5},
		{"size", QJsonArray{width, height} },
		{"stale", m_count%2==0},    // find out use of this flag in client
		{"workTime", 77}            // send actual work time ?
	};
	auto imgDescriptorHeaderObj = QJsonObject{
		{"wslink", "1.0"},
		{"id", "publish:viewport.image.push.subscription:0"},
		{"result", imgDescriptorObj }
	};
	sendTextMessage(QJsonDocument{ imgDescriptorHeaderObj }.toJson(), client);
	client->flush();

	m_count++;
}

void iAWebsocketAPI::sendViewIDUpdate(std::shared_ptr<iAJPGImage> img, QString viewID)
{
	if (!setRenderedImage(img, viewID))
	{
		return;
	}
	if (subscriptions.contains(viewID))
	{
		for (auto client : subscriptions[viewID])
		{
			sendImage(client, viewID);
		}
	}
}

/*
void iAWebsocketAPI::processBinaryMessage(QByteArray message)
{
	QWebSocket* client = qobject_cast<QWebSocket*>(sender());
	LOG(lvlDebug, QString("Binary Message received!"));
}
*/

void iAWebsocketAPI::socketDisconnected()
{
	QWebSocket* client = qobject_cast<QWebSocket*>(sender());
	if (!client)
	{
		LOG(lvlWarn, "Invalid call to socketDisconnected with nullptr client!");
		return;
	}
	LOG(lvlDebug, QString("Client disconnected: %1 (local %2)").arg(client->peerAddress().toString()).arg(client->localAddress().toString()));
	for (auto &x : subscriptions)
	{
		x.removeAll(client);
	}
	auto it = findClient(m_clients, client);
	client->deleteLater();
	if (it == m_clients.end())
	{
		LOG(lvlWarn, "Disconnect from socket not found in list of connected clients!");
		return;
	}
	emit clientDisconnected(it->id);
	m_clients.erase(it);
}

void iAWebsocketAPI::updateCaptionList(std::vector<iAAnnotation> captions)
{
	QJsonArray captionList;
	for (auto caption : captions)
	{
		captionList.append(QJsonObject{
			{"id", static_cast<int>(caption.m_id) },
			{"Title", caption.m_name},
			{"x", caption.m_coord[0]},
			{"y", caption.m_coord[1]},
			{"z", caption.m_coord[2]},
			{"hide", !caption.m_show}
		});
	}
	auto response = QJsonObject{
		{"id", "caption.response"},
		{"captionList", captionList}
	};
	m_captionUpdate = QJsonDocument{ response };
	sendCaptionUpdate();
}

void iAWebsocketAPI::captionSubscribe(QWebSocket* client)
{
	if (subscriptions.contains(captionKey))
	{
		subscriptions[captionKey].append(client);
	}
	else
	{
		subscriptions.insert(captionKey, QList<QWebSocket*>());
		subscriptions[captionKey].append(client);
	}
	sendCaptionUpdate();
}

void iAWebsocketAPI::sendCaptionUpdate()
{
	if (subscriptions.contains(captionKey))
	{
		for (auto client : subscriptions[captionKey])
		{
			sendTextMessage(m_captionUpdate.toJson(), client);
		}
	}
}

void iAWebsocketAPI::sendInteractionUpdate( size_t focusedId)
{
	auto response = QJsonObject{
		{"id", "caption.interactionUpdate"},
		{"focusedId", static_cast<int>(focusedId)}
	};
	const QJsonDocument JsonResponse{response};

	if (subscriptions.contains(captionKey))
	{
		for (auto client : subscriptions[captionKey])
		{
			sendTextMessage(JsonResponse.toJson(), client);
		}
	}
}
