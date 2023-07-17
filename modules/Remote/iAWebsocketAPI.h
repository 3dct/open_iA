// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAAnnotationTool.h>

#include <QByteArray>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QObject>

#include <mutex>

class iARemoteAction;

class QWebSocket;
class QWebSocketServer;

class iAWebsocketAPI : public QObject
{ 
	Q_OBJECT
public:
	iAWebsocketAPI(quint16 port);
	//! retrieve all queued actions, empty this object's queue in the process
	QList<iARemoteAction*> getQueuedActions();

public Q_SLOTS:
	void init();
	void close();
	//! call when a new image is available which should be sent out to clients (calls setRenderedImage internally)
	void sendViewIDUpdate(QByteArray img, QString viewID);
	void updateCaptionList(std::vector<iAAnnotation> captions);
	void sendInteractionUpdate(size_t focusedId);
	//! call when a new image is available which doesn't need to be sent out immediately (called by sendViewIDUpdate internally)
	//! @return true if passed in image is new, false if passed in image is the same as was cached in previous call
	bool setRenderedImage(QByteArray img, QString viewID);
	 
Q_SIGNALS:
	void closed();
	void controlCommand();
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
	
private:
	quint16 m_port;
	QWebSocketServer* m_wsServer;
	QList<QWebSocket*> m_clients;
	QMap<QString, QList<QWebSocket*>> subscriptions;
	int m_count;
	QMap<QString, QByteArray> images;
	QJsonDocument m_captionUpdate;
	const QString captionKey = "caption";
	QElapsedTimer m_StoppWatch;

	QList<iARemoteAction*> m_actions;
	std::mutex m_actionsMutex;

	void commandWslinkHello(QJsonDocument Request, QWebSocket* pClient);
	void commandAddObserver(QJsonDocument Request, QWebSocket* pClient);
	void commandImagePush(QJsonDocument Request, QWebSocket* pClient);
	void commandImagePushSize(QJsonDocument Request, QWebSocket* pClient);
	void commandImagePushInvalidateCache(QJsonDocument Request, QWebSocket* pClient);
	void commandImagePushQuality(QJsonDocument Request, QWebSocket* pClient);
	void commandControls(QJsonDocument Request, QWebSocket* pClient);

	void sendSuccess(QJsonDocument Request, QWebSocket* pClient);
	void sendImage(QWebSocket* pClient, QString viewID);
};
