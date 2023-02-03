// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>
#include <QObject>

#include <memory>

class iAViewHandler;
class iAWebsocketAPI;

class vtkRenderWindow;

class iARemoteRenderer: public QObject
{
Q_OBJECT

public:
	iARemoteRenderer(int port);

	void addRenderWindow(vtkRenderWindow* window, QString const& viewID);
	void removeRenderWindow(QString const& viewID);
	vtkRenderWindow* renderWindow(QString const& viewID);

	std::unique_ptr<iAWebsocketAPI> m_websocket;

private:
	QMap<QString, vtkRenderWindow*> m_renderWindows;
	long long Lastrendered=0;
	int timeRendering;
	QMap<QString, iAViewHandler*> views;

public Q_SLOTS: 
	void createImage(QString const& ViewID, int Quality );

Q_SIGNALS:
	void imageHasChanged(QByteArray Image, QString ViewID);
};
