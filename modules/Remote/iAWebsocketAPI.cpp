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
	if (m_pWebSocketServer->listen(QHostAddress::Any, port))
	{
		connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &iAWebsocketAPI::onNewConnection);
		connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &iAWebsocketAPI::closed);
	}
}

iAWebsocketAPI::~iAWebsocketAPI()
{
	m_pWebSocketServer->close();
	qDeleteAll(m_clients.begin(), m_clients.end());
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
	QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());

	auto Request = QJsonDocument::fromJson(message.toLatin1());


	if (pClient && Request["method"].toString() == "wslink.hello")
	{
		const auto ClientID = QJsonObject{{"clientID", "123456789"}};
		const auto resultArray = QJsonArray{ClientID};

		QJsonObject ResponseArray;

		ResponseArray["wslink"] = "1.0";
		ResponseArray["id"] = Request["id"].toString();
		ResponseArray["result"] = ClientID;

		const QJsonDocument Response{ResponseArray};

		pClient->sendTextMessage(Response.toJson());
	}
	else
	{
		QJsonObject ResponseArray;

		ResponseArray["wslink"] = "1.0";
		QString viewIDString = Request["View"].toString();
		
		const auto success = QJsonObject{{"result", "success"}};
		if (Request["method"].toString() == "viewport.image.push.observer.add")
		{

			const auto viewIDResponse = QJsonObject{{"result", "success"}, {"viewId", viewIDString}};
			ResponseArray["result"] = viewIDResponse;
		}
		else
		{
			ResponseArray["result"] = success;
		}

		const QJsonDocument Response{ResponseArray};

		pClient->sendTextMessage(Response.toJson());

		if (Request["method"].toString() == "viewport.image.push")
		{
			sendImage(pClient);
		}
	}
}

void iAWebsocketAPI::sendImage(QWebSocket* pClient)
{
	QString imageString("wslink_bin");
	
	imageString.append(QString::number(m_count));

	const auto resultArray1 = QJsonArray{imageString};
	//const auto resultArray = QJsonArray{ClientID};

	QJsonObject ResponseArray;

	ResponseArray["wslink"] = "1.0";
	ResponseArray["method"] = "wslink.binary.attachment";
	ResponseArray["args"] = resultArray1;

	const QJsonDocument Response{ResponseArray};

	pClient->sendTextMessage(Response.toJson());


	QImage img("C:\\Users\\p41877\\Pictures\\cat.jpg");
	//QImage img2 = img.scaled(1920, 872);

	//img2.save("C:\\Users\\P41877\\Downloads\\catImage2.jpg");

	QByteArray ba;

	QFile file("C:\\Users\\p41877\\Pictures\\cat.jpg");
	QDataStream in(&file);
	file.open(QIODevice::ReadOnly);

	ba.resize(file.size());
	in.readRawData(ba.data(), file.size());
	pClient->sendBinaryMessage(ba);

	auto imageSize = ba.size();

	const auto resultArray2 = QJsonArray{img.size().width(), img.size().height()};
	const auto result = QJsonObject{{"format", "jpeg"}, {"global_id", 1}, {"global_id", "1"}, {"id", "1"},
		{"image", imageString}, {"localTime", 0}, {"memsize", imageSize}, {"mtime", 2125+m_count*5}, {"size", resultArray2},
		{"stale", m_count%2==0}, {"workTime", 77}};
	

	QJsonObject ResponseArray2;

	ResponseArray2["wslink"] = "1.0";
	ResponseArray2["id"] = "publish:viewport.image.push.subscription:0";
	ResponseArray2["result"] = result;

	const QJsonDocument Response2{ResponseArray2};

	pClient->sendTextMessage(Response2.toJson());

	m_count++;

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
		m_clients.removeAll(pClient);
		pClient->deleteLater();
	}
}
