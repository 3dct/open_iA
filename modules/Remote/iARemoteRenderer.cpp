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
#include "iALog.h"
#include "vtkUnsignedCharArray.h"

#include <vtkRendererCollection.h>
#include <vtkCallbackCommand.h>

iARemoteRenderer::iARemoteRenderer(int port)
{

	m_websocket = new iAWebsocketAPI(port);
	connect(this, &iARemoteRenderer::imageHasChanged, m_websocket, &iAWebsocketAPI::sendViewIDUpdate);


}

void iARemoteRenderer::addRenderWindow(vtkRenderWindow* window, QString viewID)
{
	m_renderWindows.insert(viewID, window);
	auto data = iAImagegenerator::createImage(window,100);
	QByteArray img((char*)data->Begin(), static_cast<qsizetype>(data->GetSize()));
	m_websocket->setRenderedImage(img, viewID);

	auto view = new viewHandler();
	view->id = viewID;

	connect(view, &viewHandler::createImage, this, &iARemoteRenderer::createImage);

	views.insert(viewID,view);
	auto renderer = window->GetRenderers()->GetFirstRenderer();
	renderer->AddObserver(vtkCommand::EndEvent, view, &viewHandler::vtkCallbackFunc);


}



void iARemoteRenderer::removeRenderWindow(QString viewID)
{
	m_renderWindows.remove(viewID);
}

void iARemoteRenderer::createImage(QString ViewID, int Quality)
{
	auto data = iAImagegenerator::createImage(m_renderWindows[ViewID],Quality);
	QByteArray img((char*)data->Begin(), static_cast<qsizetype>(data->GetSize()));

	imageHasChanged(img, ViewID);
}

