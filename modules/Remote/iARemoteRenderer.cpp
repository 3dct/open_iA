// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemoteRenderer.h"

#include "iAImagegenerator.h"
#include "iAViewHandler.h"
#include "iAWebsocketAPI.h"

#include <vtkCallbackCommand.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkUnsignedCharArray.h>

#include <QThread>

iARemoteRenderer::iARemoteRenderer(int port):
	m_wsAPI(std::make_unique<iAWebsocketAPI>(port)),
	m_wsThread(std::make_unique<QThread>())
{
	m_wsThread->setObjectName("WebSocketServer");
	m_wsAPI->moveToThread(m_wsThread.get());
	connect(m_wsThread.get(), &QThread::started, m_wsAPI.get(), &iAWebsocketAPI::init);
	connect(this, &iARemoteRenderer::imageHasChanged, m_wsAPI.get(), &iAWebsocketAPI::sendViewIDUpdate);
	connect(this, &iARemoteRenderer::finished, m_wsAPI.get(), &iAWebsocketAPI::close);
	connect(this, &iARemoteRenderer::setRenderedImage, m_wsAPI.get(), &iAWebsocketAPI::setRenderedImage);
	m_wsThread->start();
}

iARemoteRenderer::~iARemoteRenderer()
{
	emit finished();
	m_wsThread->quit();
	m_wsThread->wait();
}

void iARemoteRenderer::addRenderWindow(vtkRenderWindow* window, QString const& viewID)
{
	m_renderWindows.insert(viewID, window);
	auto imgData = iAImagegenerator::createImage(viewID, window, 100);
	emit setRenderedImage(imgData, viewID);

	auto view = new iAViewHandler(viewID);
	connect(view, &iAViewHandler::createImage, this, &iARemoteRenderer::createImage);

	views.insert(viewID, view);
	auto renderer = window->GetRenderers()->GetFirstRenderer();
	renderer->AddObserver(vtkCommand::EndEvent, view, &iAViewHandler::vtkCallbackFunc);
}

void iARemoteRenderer::removeRenderWindow(QString const &viewID)
{
	m_renderWindows.remove(viewID);
}

vtkRenderWindow* iARemoteRenderer::renderWindow(QString const& viewID)
{
	return m_renderWindows[viewID];
}

void iARemoteRenderer::createImage(QString const& viewID, int quality)
{
	QSignalBlocker block(views[viewID]);
	auto data = iAImagegenerator::createImage(viewID, m_renderWindows[viewID], quality);
	emit imageHasChanged(data, viewID);
}
