/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAWebsocketAPI.h"

#include <iALog.h>

#include "iARemoteAction.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QImage>




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

void iAWebsocketAPI::setRenderedImage(QByteArray img, QString id)
{
	images.insert(id,img);
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

	LOG(lvlDebug, QString("Websocket time %1").arg(m_StoppWatch.elapsed()));
	m_StoppWatch.restart();

	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());

	auto Request = QJsonDocument::fromJson(message.toLatin1());


	if (Request["method"].toString() == "wslink.hello")
	{ 
		commandWslinkHello(Request, pClient);
	}
	else if (Request["method"].toString() == "viewport.image.push.observer.add")
	{
		commandAdObserver(Request, pClient);
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

	QJsonObject ResponseArray;

	ResponseArray["wslink"] = "1.0";
	ResponseArray["id"] = Request["id"].toString();
	ResponseArray["result"] = ClientID;

	const QJsonDocument Response{ResponseArray};

	pClient->sendTextMessage(Response.toJson());
}

void iAWebsocketAPI::commandAdObserver(QJsonDocument Request, QWebSocket* pClient)
{
	QJsonObject ResponseArray;

	ResponseArray["wslink"] = "1.0";
	QString viewIDString = Request["args"][0].toString();
	const auto viewIDResponse = QJsonObject{{"result", "success"}, {"viewId", viewIDString}};
	ResponseArray["result"] = viewIDResponse;
	const QJsonDocument Response{ResponseArray};

	if (subscriptions.contains(viewIDString))
	{
		subscriptions[viewIDString].append(pClient);
	}
	else
	{
		subscriptions.insert(viewIDString,  QList<QWebSocket*>());
		subscriptions[viewIDString].append(pClient);
	}
	
	pClient->sendTextMessage(Response.toJson());
}


void iAWebsocketAPI::commandImagePush(QJsonDocument Request, QWebSocket* pClient)
{
	QString viewIDString = Request["args"][0]["view"].toString();
	commandImagePushSize(Request, pClient);
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
	QJsonObject ResponseArray;
	const auto success = QJsonObject{{"result", "success"}};
	ResponseArray["wslink"] = "1.0";
	ResponseArray["result"] = success;
	const QJsonDocument Response{ResponseArray};

	pClient->sendTextMessage(Response.toJson());
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



void iAWebsocketAPI::sendImage(QWebSocket* pClient, QString viewID)  // use in future
{
	QString imageString("wslink_bin");
	
	imageString.append(QString::number(m_count));

	const auto resultArray1 = QJsonArray{imageString};

	QJsonObject ResponseArray;

	ResponseArray["wslink"] = "1.0";
	ResponseArray["method"] = "wslink.binary.attachment";
	ResponseArray["args"] = resultArray1;

	const QJsonDocument Response{ResponseArray};

	pClient->sendTextMessage(Response.toJson());

	

	int width=0, height=0;

	
	QByteArray ba = images[viewID];
	QImage img;
	img.loadFromData(ba);
	width = img.size().width();
	height = img.size().height();
	pClient->sendBinaryMessage(ba);

	auto imageSize = ba.size();

	const auto resultArray2 = QJsonArray{width, height};
	const auto result = QJsonObject{{"format", "jpeg"}, {"global_id", 1}, {"global_id", "1"}, {"id", viewID},
		{"image", imageString}, {"localTime", 0}, {"memsize", imageSize}, {"mtime", 2125+m_count*5}, {"size", resultArray2},
		{"stale", m_count%2==0}, {"workTime", 77}};
	

	QJsonObject ResponseArray2;

	ResponseArray2["wslink"] = "1.0";
	ResponseArray2["id"] = "publish:viewport.image.push.subscription:0";
	ResponseArray2["result"] = result;

	const QJsonDocument Response2{ResponseArray2};

	pClient->sendTextMessage(Response2.toJson());

	pClient->flush();

	m_count++;

}

void iAWebsocketAPI::sendViewIDUpdate(QByteArray img, QString ViewID)
{

	setRenderedImage(img, ViewID);

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

void iAWebsocketAPI::updateCaptionList(std::vector<iAAnnotation> captions)
{

	QJsonArray captionList;

	for (auto caption : captions)
	{
		QJsonObject captionObject;
		captionObject["id"] = (int)caption.m_id;
		captionObject["Title"] = caption.m_name;
		captionObject["x"] = caption.m_coord[0];
		captionObject["y"] = caption.m_coord[1];
		captionObject["z"] = caption.m_coord[2];
		captionObject["hide"] = caption.m_hide;

		captionList.append(captionObject);

	}

	QJsonObject response;
	response["id"] = "caption.response";
	response["captionList"] = captionList;
	const QJsonDocument Response2{response};

	m_captionUpdate = Response2;

	sendCaptionUpdate();

}

void iAWebsocketAPI::captionSubscribe(QWebSocket* pClient)
{


	if (subscriptions.contains(cptionKey))
	{
		subscriptions[cptionKey].append(pClient);
	}
	else
	{
		subscriptions.insert(cptionKey, QList<QWebSocket*>());
		subscriptions[cptionKey].append(pClient);
	}
	sendCaptionUpdate();
}


void iAWebsocketAPI::sendCaptionUpdate()
{


	if (subscriptions.contains(cptionKey))
	{
		for (auto client : subscriptions[cptionKey])
		{
			client->sendTextMessage(m_captionUpdate.toJson());
		}
	}
}

void iAWebsocketAPI::sendInteractionUpdate( size_t focusedId)
{

	QJsonObject response;
	response["id"] = "caption.interactionUpdate";
	response["focusedId"] = (int)focusedId;
	const QJsonDocument JsonResponse{response};

	if (subscriptions.contains(cptionKey))
	{
		for (auto client : subscriptions[cptionKey])
		{
			client->sendTextMessage(JsonResponse.toJson());
		}
	}
}