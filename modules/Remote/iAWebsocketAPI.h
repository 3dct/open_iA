// Copyright (c) open_iA contributors
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

class iAJPGImage;
class iARemoteAction;

class QWebSocket;
class QWebSocketServer;

class iAWSClient
{
public:
	QWebSocket* ws;   //!< the websocket connection
	int id;    //!< internal id of the client
	quint64 rcvd;  //!< bytes received
	quint64 sent;  //!< bytes sent
};

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
	void sendViewIDUpdate(std::shared_ptr<iAJPGImage> img, QString viewID);
	void updateCaptionList(std::vector<iAAnnotation> captions);
	void sendInteractionUpdate(size_t focusedId);
	//! call when a new image is available which doesn't need to be sent out immediately (called by sendViewIDUpdate internally)
	//! @return true if passed in image is new, false if passed in image is the same as was cached in previous call
	bool setRenderedImage(std::shared_ptr<iAJPGImage> img, QString viewID);
	 
Q_SIGNALS:
	void closed();
	void controlCommand();
	void addMode();
	void removeCaption(int capID);
	void selectCaption(int capID);
	void changeCaptionTitle(int capID, QString title);
	void hideCaption(int capID);

	void clientConnected(int clientID);
	void clientDisconnected(int clientID);
	void clientTransferUpdated(int clientID, quint64 rcvd, quint64 sent);
	
private Q_SLOTS:
	void onNewConnection();
	void processTextMessage(QString message);
	//void processBinaryMessage(QByteArray message);
	void socketDisconnected();

	void captionSubscribe(QWebSocket* client);
	void sendCaptionUpdate();
	
private:
	quint16 m_port;
	QWebSocketServer* m_wsServer;
	QList<iAWSClient> m_clients;
	QMap<QString, QList<QWebSocket*>> subscriptions;
	int m_count;
	QMap<QString, std::shared_ptr<iAJPGImage>> images;
	QJsonDocument m_captionUpdate;
	const QString captionKey = "caption";
	QElapsedTimer m_StoppWatch;
	QList<iARemoteAction*> m_actions;
	std::mutex m_actionsMutex;
	int m_clientID;

	void commandWslinkHello(QJsonDocument request, QWebSocket* client);
	void commandAddObserver(QJsonDocument request, QWebSocket* client);
	void commandImagePush(QJsonDocument request, QWebSocket* client);
	void commandImagePushSize(QJsonDocument request, QWebSocket* client);
	void commandImagePushInvalidateCache(QJsonDocument request, QWebSocket* client);
	void commandImagePushQuality(QJsonDocument request, QWebSocket* client);
	void commandControls(QJsonDocument request, QWebSocket* client);

	void sendSuccess(QJsonDocument request, QWebSocket* client);
	void sendImage(QWebSocket* client, QString viewID);
	
	void sendTextMessage(QByteArray const& data, QWebSocket* client);
	void sendBinaryMessage(QByteArray const& data, QWebSocket* client);
};
