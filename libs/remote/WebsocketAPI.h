#pragma once

#include "iAremote_export.h"

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QByteArray>

QT_USE_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

class WebsocketAPI : public QObject
{
	Q_OBJECT
public:
	 WebsocketAPI(quint16 port, bool debug = false, QObject* parent = nullptr);
	~WebsocketAPI();

Q_SIGNALS:
	void closed();

private Q_SLOTS:
	void onNewConnection();
	void processTextMessage(QString message);
	void sendImage(QWebSocket* pClient);
	void processBinaryMessage(QByteArray message);
	void socketDisconnected();

private:
	QWebSocketServer* m_pWebSocketServer;
	QList<QWebSocket*> m_clients;
	bool m_debug;
	int m_count;
};
