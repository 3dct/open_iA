// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
