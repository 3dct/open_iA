// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>
#include <QObject>

#include <memory>

class iAJPGImage;
class iAViewHandler;
class iAWebsocketAPI;

class vtkRenderWindow;

class QThread;

class iARemoteRenderer: public QObject
{
Q_OBJECT

public:
	iARemoteRenderer(int port);
	~iARemoteRenderer();
	//! start the websocket listening in separate thread
	void start();
	void addRenderWindow(vtkRenderWindow* window, QString const& viewID);
	void removeRenderWindow(QString const& viewID);
	vtkRenderWindow* renderWindow(QString const& viewID);
	std::unique_ptr<iAWebsocketAPI> m_wsAPI;

private:
	QMap<QString, vtkRenderWindow*> m_renderWindows;
	QMap<QString, iAViewHandler*> views;
	std::unique_ptr<QThread> m_wsThread;

public slots:
	void createImage(QString const& viewID, int quality );

signals:
	//! Called when image for a view changes;
	//! used to communicate with web socket server thread
	bool setRenderedImage(std::shared_ptr<iAJPGImage> img, QString viewID);
	//! Called when image for a view changes;
	//! used to communicate with web socket server thread
	void imageHasChanged(std::shared_ptr<iAJPGImage> img, QString viewID);
};
