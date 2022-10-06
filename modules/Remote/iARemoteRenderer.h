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
#pragma once

#include "iAWebsocketAPI.h"

#include <QMap>
#include <QList>
#include <QTime>
#include <QTimer>

#include <vtkRenderWindow.h>


class iARemoteRenderer: public QObject
{

Q_OBJECT

public:
	iARemoteRenderer(int port);

	iAWebsocketAPI* m_websocket;

	void addRenderWindow(vtkRenderWindow* window, QString viewID);

	void vtkCallbackFunc(vtkObject* caller, long unsigned int evId, void*);

	void removeRenderWindow(QString viewID);

	

	



private:
	QMap<QString, vtkRenderWindow*> m_renderWindows;
	long long Lastrendered=0;
	int timeRendering;
	QTimer* timer;

public Q_SLOTS: 
	void createImage(QString ViewID, int Quality = 50);

Q_SIGNALS:
	void imageHasChanged(QByteArray Image, QString ViewID);

};

