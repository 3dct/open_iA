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
#pragma once

#include "RemoteAction.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QMap>

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

class iAWebsocketAPI : public QObject
{
	Q_OBJECT
public:
	iAWebsocketAPI(quint16 port, bool debug = false, QObject* parent = nullptr);
	void setRenderedImage(QByteArray img, QString id);
	~iAWebsocketAPI();

Q_SIGNALS:
	void closed();
	void controlComand(RemoteAction const & action);

private Q_SLOTS:
	void onNewConnection();
	void processTextMessage(QString message);
	void processBinaryMessage(QByteArray message);
	void socketDisconnected();
public Q_SLOTS:
	void sendViewIDUpdate(QByteArray img, QString ViewID);


private:
	QWebSocketServer* m_pWebSocketServer;
	QList<QWebSocket*> m_clients;
	bool m_debug;
	int m_count;
	QMap<QString, QByteArray> images;

	QMap<QString, QList<QWebSocket*>> subscriptions;

	void ComandWslinkHello(QJsonDocument Request, QWebSocket* pClient);
	void ComandAdObserver(QJsonDocument Request, QWebSocket* pClient);
	void ComandImagePush(QJsonDocument Request, QWebSocket* pClient);
	void ComandImagePushSize(QJsonDocument Request, QWebSocket* pClient);
	void ComandImagePushInvalidateCache(QJsonDocument Request, QWebSocket* pClient);
	void ComandImagePushQuality(QJsonDocument Request, QWebSocket* pClient);
	void sendSuccess(QJsonDocument Request, QWebSocket* pClient);
	void ComandControls(QJsonDocument Request, QWebSocket* pClient);  

	void sendImage(QWebSocket* pClient, QString viewID);

	
	

};
