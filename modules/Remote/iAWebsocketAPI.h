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

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QMap>
#include <QThread>
#include <iACaptionItem.h>
#include <QJsonDocument>

#include <iAAnnotationTool.h>

class iARemoteAction;

class QWebSocket;
class QWebSocketServer;

class iAWebsocketAPI : public QObject
{ 
	Q_OBJECT
public:
	iAWebsocketAPI(quint16 port, bool debug = false, QObject* parent = nullptr);
	void setRenderedImage(QByteArray img, QString id);
	~iAWebsocketAPI();
	 
Q_SIGNALS:
	void closed();
	void controlCommand(iARemoteAction const & action);
	void removeCaption(int id);
	void addMode();
	void selectCaption(int id);
	void changeCaptionTitle(int id, QString title); 
	void hideAnnotation(int id);
	
private Q_SLOTS:
	void onNewConnection();
	void processTextMessage(QString message);
	void processBinaryMessage(QByteArray message);
	void socketDisconnected();
	void captionSubscribe(QWebSocket* pClient);

	void sendCaptionUpdate();
	



public Q_SLOTS:
	void sendViewIDUpdate(QByteArray img, QString ViewID);
	void updateCaptionList(std::vector<iAAnnotation> captions);
	void sendInteractionUpdate( size_t focusedId);
	
 

private:
	QWebSocketServer* m_pWebSocketServer;
	QList<QWebSocket*> m_clients;
	bool m_debug;
	int m_count;
	QMap<QString, QByteArray> images;
	QJsonDocument m_captionUpdate;
	const QString cptionKey = "caption";

	QElapsedTimer m_StoppWatch;

	QThread m_ServerThread;

	QMap<QString, QList<QWebSocket*>> subscriptions;

	void commandWslinkHello(QJsonDocument Request, QWebSocket* pClient);
	void commandAdObserver(QJsonDocument Request, QWebSocket* pClient);
	void commandImagePush(QJsonDocument Request, QWebSocket* pClient);
	void commandImagePushSize(QJsonDocument Request, QWebSocket* pClient);
	void commandImagePushInvalidateCache(QJsonDocument Request, QWebSocket* pClient);
	void commandImagePushQuality(QJsonDocument Request, QWebSocket* pClient);
	void commandControls(QJsonDocument Request, QWebSocket* pClient);

	void sendSuccess(QJsonDocument Request, QWebSocket* pClient);
	void sendImage(QWebSocket* pClient, QString viewID);

};
