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
	auto imgData = iAImagegenerator::createImage(window,100);
	m_websocket->setRenderedImage(imgData, viewID);

	auto view = new iAViewHandler();
	view->id = viewID;

	connect(view, &iAViewHandler::createImage, this, &iARemoteRenderer::createImage);

	views.insert(viewID,view);
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

void iARemoteRenderer::createImage(QString const& ViewID, int Quality)
{
	QSignalBlocker block(views[ViewID]);
	auto data = iAImagegenerator::createImage(m_renderWindows[ViewID],Quality);

	imageHasChanged(data, ViewID);
}
