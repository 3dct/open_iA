// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <QObject>

class iAMainWindow;
class iAMdiChild;
class iAQVTKWidget;
class iARemoteRenderer;

class QHttpServer;

class iARemoteTool : public QObject, public iATool
{
	Q_OBJECT
public:
	static const QString Name;
	iARemoteTool(iAMainWindow* mainWnd, iAMdiChild* child);
	~iARemoteTool();

private slots:
	void init();

private:
	std::unique_ptr<iARemoteRenderer> m_remoteRenderer;
#ifdef QT_HTTPSERVER
	std::unique_ptr<QHttpServer> m_httpServer;
#endif
};
