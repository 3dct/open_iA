// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemoteRenderer.h"

#include "iAImagegenerator.h"
#include "iAViewHandler.h"
#include "iAWebsocketAPI.h"

#include <vtkCallbackCommand.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkUnsignedCharArray.h>

iARemoteRenderer::iARemoteRenderer(int port):
	m_websocket(std::make_unique<iAWebsocketAPI>(port))
{
	connect(this, &iARemoteRenderer::imageHasChanged, m_websocket.get(), &iAWebsocketAPI::sendViewIDUpdate);
}

void iARemoteRenderer::addRenderWindow(vtkRenderWindow* window, QString const& viewID)
{
	m_renderWindows.insert(viewID, window);
	auto imgData = iAImagegenerator::createImage(viewID, window, 100);
	m_websocket->setRenderedImage(imgData, viewID);

	auto view = new iAViewHandler();
	view->id = viewID;
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
