// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAWebsocketAPI.h"

#include "iARemoteAction.h"

#include <iALog.h>

#include <QDebug>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocketServer>
#include <QWebSocket>

iAWebsocketAPI::iAWebsocketAPI(quint16 port, bool debug, QObject* parent) :
	QObject(parent),
	m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Remote Server"), QWebSocketServer::NonSecureMode, this)),
	m_debug(debug)
{
	m_pWebSocketServer->moveToThread(&m_ServerThread);
	m_ServerThread.start();

	if (m_pWebSocketServer->listen(QHostAddress::Any, port))
	{
		connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &iAWebsocketAPI::onNewConnection);
		connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &iAWebsocketAPI::closed);
	}

	std::vector<iAAnnotation> captions;
	updateCaptionList(captions);
}

bool iAWebsocketAPI::setRenderedImage(QByteArray img, QString id)
{
	if (images.contains(id) && images[id] == img)
	{
		LOG(lvlDebug, "Setting same image again, ignoring!");
		return false;
	}
	images.insert(id, img);
	return true;
}

iAWebsocketAPI::~iAWebsocketAPI()
{
	m_ServerThread.quit();
	m_ServerThread.wait();
	//qDeleteAll(m_clients);	// memory leak? but with it, we crash with access violation!
	m_pWebSocketServer->close();
}

void iAWebsocketAPI::onNewConnection()
{
	m_count = 0;
	QWebSocket* pSocket = m_pWebSocketServer->nextPendingConnection();
	connect(pSocket, &QWebSocket::textMessageReceived, this, &iAWebsocketAPI::processTextMessage);
	connect(pSocket, &QWebSocket::binaryMessageReceived, this, &iAWebsocketAPI::processBinaryMessage);
	connect(pSocket, &QWebSocket::disconnected, this, &iAWebsocketAPI::socketDisconnected);
	m_clients << pSocket;
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

void iAWebsocketAPI::commandControls(QJsonDocument Request, QWebSocket* pClient)
{
	iARemoteAction webAction ;
	auto argList = Request["args"][0];

	if (argList["action"] == "down")
	{
		webAction.action = iARemoteAction::ButtonDown;
	}
	else if (argList["action"] == "up")
	{
		webAction.action = iARemoteAction::ButtonUp;
	}
	else if (argList["type"] == "MouseWheel")
	{
		webAction.action = iARemoteAction::MouseWheel;
	}
	else if (argList["type"] == "EndMouseWheel")
	{
		webAction.action = iARemoteAction::EndMouseWheel;
	}
	else if (argList["type"] == "StartMouseWheel")
	{
		webAction.action = iARemoteAction::StartMouseWheel;
	}
	else
	{
		webAction.action = iARemoteAction::Unknown;
	}

	webAction.buttonLeft = argList["buttonLeft"].toInt();
	webAction.buttonRight = argList["buttonRight"].toInt();
	webAction.buttonMiddle = argList["buttonMiddle"].toInt();
	webAction.altKey = argList["altKey"].toInt();
	webAction.ctrlKey = argList["controlKey"].toBool() || argList["ctrlKey"].toInt();
	webAction.metaKey = argList["metaKey"].toInt();
	webAction.shiftKey = argList["shiftKey"].toInt() || argList["shiftKey"].toBool();
	webAction.viewID = argList["view"].toString();
	webAction.x = argList["x"].toDouble();
	webAction.y = argList["y"].toDouble();
	webAction.spinY = argList["spinY"].toDouble();

	emit controlCommand(webAction);

	sendSuccess(Request, pClient);
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

void iAWebsocketAPI::sendViewIDUpdate(QByteArray img, QString ViewID)
{
	if (!setRenderedImage(img, ViewID))
	{
		return;
	}
	if (subscriptions.contains(ViewID))
	{
		for (auto client : subscriptions[ViewID])
		{
			sendImage(client, ViewID);
		}
	}
}

void iAWebsocketAPI::processBinaryMessage(QByteArray message)
{
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
	if (m_debug)
	{
		qDebug() << "Binary Message received:" << message;
	}
	if (pClient)
	{
		pClient->sendBinaryMessage(message);
	}
}

void iAWebsocketAPI::socketDisconnected()
{
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
	if (m_debug)
	{
		qDebug() << "socketDisconnected:" << pClient;
	}
	if (pClient)
	{
		for (auto &x : subscriptions)
		{
			x.removeAll(pClient);
		}
		m_clients.removeAll(pClient);
		pClient->deleteLater();
	}
}

void iAWebsocketAPI::updateCaptionList(std::vector<iAAnnotation> const & captions)
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
