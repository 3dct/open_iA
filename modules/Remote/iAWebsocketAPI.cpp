// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAWebsocketAPI.h"

#include "iARemoteAction.h"

#include <iALog.h>

#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocketServer>
#include <QWebSocket>

iAWebsocketAPI::iAWebsocketAPI(quint16 port):
	m_port(port),
	m_count(0) // possibly use a separate counter per client?
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

bool iAWebsocketAPI::setRenderedImage(QByteArray img, QString viewID)
{
	// TODO: check other thread communication methods, e.g. locking images via mutex; currently using signal/slot
	if (images.contains(viewID) && images[viewID] == img)
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
	LOG(lvlDebug, QString("Client connected: %1 (local %2)").arg(client->peerAddress().toString()).arg(client->localAddress().toString()));
	connect(client, &QWebSocket::textMessageReceived, this, &iAWebsocketAPI::processTextMessage);
	connect(client, &QWebSocket::binaryMessageReceived, this, &iAWebsocketAPI::processBinaryMessage);
	connect(client, &QWebSocket::disconnected, this, &iAWebsocketAPI::socketDisconnected);
	m_clients << client;
}

void iAWebsocketAPI::processTextMessage(QString message)
{
	//LOG(lvlDebug, QString("Websocket time %1").arg(m_StoppWatch.elapsed()));
	m_StoppWatch.restart();

	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());

	auto Request = QJsonDocument::fromJson(message.toLatin1());

	if (Request["method"].toString() == "wslink.hello")
	{ 
		commandWslinkHello(Request, pClient);
	}
	else if (Request["method"].toString() == "viewport.image.push.observer.add")
	{
		commandAddObserver(Request, pClient);
	}
	else if (Request["method"].toString() == "viewport.image.push")
	{
		commandImagePush(Request, pClient);
	}
	else if (Request["method"].toString() == "viewport.image.push.original.size")
	{
		commandImagePushSize(Request, pClient);
	}
	else if (Request["method"].toString() == "viewport.image.push.invalidate.cache")
	{
		commandImagePushInvalidateCache(Request, pClient);
	}
	else if (Request["method"].toString() == "viewport.image.push.quality")
	{
		commandImagePushQuality(Request, pClient);
	}
	else if (Request["method"].toString() == "viewport.mouse.interaction")
	{
		commandControls(Request, pClient);
	}
	else if (Request["method"].toString() == "viewport.mouse.zoom.wheel")
	{
		commandControls(Request, pClient);
	}

	//Captions API 
	else if (Request["method"].toString() == "subscribe.captions")
	{
		captionSubscribe(pClient);
	}
	else if (Request["method"].toString() == "select.caption")
	{
		emit selectCaption(Request["id"].toInt());
	}
	else if (Request["method"].toString() == "remove.caption")
	{
		emit removeCaption(Request["id"].toInt());
	}
	else if (Request["method"].toString() == "addMode.caption")
	{
		emit addMode();
	}
	else if (Request["method"].toString() == "nameChanged.caption")
	{
		emit changeCaptionTitle(Request["id"].toInt(), Request["title"].toString());
	}
	else if (Request["method"].toString() == "hideAnnotation.caption")
	{
		emit hideAnnotation(Request["id"].toInt());
	}
}

void iAWebsocketAPI::commandWslinkHello(QJsonDocument Request, QWebSocket* pClient)
{
	const auto ClientID = QJsonObject{{"clientID", QUuid::createUuid().toString()}};
	const auto resultArray = QJsonArray{ClientID};
	auto wsLinkHelloResponse = QJsonObject{
		{"wslink", "1.0" },
		{"id", Request["id"].toString()},
		{"result", ClientID}
	};
	pClient->sendTextMessage(QJsonDocument{ wsLinkHelloResponse }.toJson());
}

void iAWebsocketAPI::commandAddObserver(QJsonDocument Request, QWebSocket* pClient)
{
	QString viewIDString = Request["args"][0].toString();
	const auto viewIDResponse = QJsonObject{
		{"result", "success"},
		{"viewId", viewIDString}
	};
	auto addObserverResponse = QJsonObject{
		{"wslink", "1.0"},
		{"result", viewIDResponse}
	};
	pClient->sendTextMessage(QJsonDocument{ addObserverResponse }.toJson());

	if (subscriptions.contains(viewIDString))
	{
		subscriptions[viewIDString].append(pClient);
	}
	else
	{
		subscriptions.insert(viewIDString,  QList<QWebSocket*>());
		subscriptions[viewIDString].append(pClient);
	}
}

void iAWebsocketAPI::commandImagePush(QJsonDocument Request, QWebSocket* pClient)
{
	sendSuccess(Request, pClient);
	QString viewIDString = Request["args"][0]["view"].toString();
	sendImage(pClient, viewIDString);
}

void iAWebsocketAPI::commandImagePushSize(QJsonDocument Request, QWebSocket* pClient)
{
	sendSuccess(Request, pClient);
}

void iAWebsocketAPI::commandImagePushInvalidateCache(QJsonDocument Request, QWebSocket* pClient)
{
	sendSuccess(Request, pClient);
}

void iAWebsocketAPI::commandImagePushQuality(QJsonDocument Request, QWebSocket* pClient)
{
	sendSuccess(Request, pClient);
}

void iAWebsocketAPI::sendSuccess(QJsonDocument Request, QWebSocket* pClient)
{
	Q_UNUSED(Request);
	auto successResponseObj = QJsonObject{
		{"wslink", "1.0"},
		{"result", QJsonObject{{"result", "success"}}}
	};
	pClient->sendTextMessage(QJsonDocument{ successResponseObj }.toJson());
}

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

void iAWebsocketAPI::commandControls(QJsonDocument Request, QWebSocket* pClient)
{
	iARemoteAction * webAction = new iARemoteAction();
	auto argList = Request["args"][0];

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

	LOG(lvlDebug, QString("Action: %1; position: %2, %3; shift: %4; ctrl: %5; alt: %6.")
		.arg(mouseActionToString(webAction->action))
		.arg(webAction->x)
		.arg(webAction->y)
		.arg(webAction->shiftKey)
		.arg(webAction->ctrlKey)
		.arg(webAction->altKey));

	{
		// enqueue in local data structure to enable receiver to apply all actions at once:
		std::lock_guard<std::mutex> guard(m_actionsMutex);
		m_actions.push_back(webAction);
	}
	// notify GUI that control message has come in:
	emit controlCommand();
	sendSuccess(Request, pClient);
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

void iAWebsocketAPI::sendImage(QWebSocket* pClient, QString viewID)
{
	auto imgIDString = QString("wslink_bin%1").arg(m_count);
	auto imgHeaderObj = QJsonObject{
		{"wslink", "1.0"},
		{"method", "wslink.binary.attachment"},
		{"args", QJsonArray{ imgIDString } }
	};
	pClient->sendTextMessage(QJsonDocument{ imgHeaderObj }.toJson());

	QByteArray const & ba = images[viewID];
	// TODO: cache image size!
	QImage img;
	img.loadFromData(ba);
	int width = img.size().width();
	int height = img.size().height();
	pClient->sendBinaryMessage(ba);

	auto imageSize = ba.size();
	const auto imgDescriptorObj = QJsonObject{
		{"format", "jpeg"},
		{"global_id", 1},
		{"global_id", "1"},
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
	pClient->sendTextMessage(QJsonDocument{ imgDescriptorHeaderObj }.toJson());
	pClient->flush();

	m_count++;
}

void iAWebsocketAPI::sendViewIDUpdate(QByteArray img, QString viewID)
{
	//LOG(lvlDebug, QString("sendViewIDUpdate %1 START").arg(viewID));
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
	//LOG(lvlDebug, QString("sendViewIDUpdate %1 END").arg(viewID));
}

void iAWebsocketAPI::processBinaryMessage(QByteArray message)
{
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
	LOG(lvlDebug, QString("Binary Message received: %1").arg(message));
	if (pClient)
	{
		pClient->sendBinaryMessage(message);
	}
}

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
	m_clients.removeAll(client);
	client->deleteLater();
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
			{"hide", caption.m_hide}
		});
	}
	auto response = QJsonObject{
		{"id", "caption.response"},
		{"captionList", captionList}
	};
	m_captionUpdate = QJsonDocument{ response };
	sendCaptionUpdate();
}

void iAWebsocketAPI::captionSubscribe(QWebSocket* pClient)
{
	if (subscriptions.contains(captionKey))
	{
		subscriptions[captionKey].append(pClient);
	}
	else
	{
		subscriptions.insert(captionKey, QList<QWebSocket*>());
		subscriptions[captionKey].append(pClient);
	}
	sendCaptionUpdate();
}

void iAWebsocketAPI::sendCaptionUpdate()
{
	if (subscriptions.contains(captionKey))
	{
		for (auto client : subscriptions[captionKey])
		{
			client->sendTextMessage(m_captionUpdate.toJson());
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
			client->sendTextMessage(JsonResponse.toJson());
		}
	}
}
