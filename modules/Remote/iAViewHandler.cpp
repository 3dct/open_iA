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
#include "iAViewHandler.h"

#include <iALog.h>

iAViewHandler::iAViewHandler() {
	timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, &QTimer::timeout, [=]() -> void {
		//LOG(lvlDebug, "TIMER");
		createImage(id, 100); });

	m_StoppWatch.start();


}

void iAViewHandler::vtkCallbackFunc(vtkObject* caller, long unsigned int evId, void* /*callData*/) {

	LOG(lvlDebug, QString("DIRECT time check %1, time %2").arg(id).arg(m_StoppWatch.elapsed()));

	if ((m_StoppWatch.elapsed() > waitTimeRendering) && (m_StoppWatch.elapsed() >50))
	{
		m_StoppWatch.restart();
		timer->stop();
		timer->start(250);

		createImage(id, quality);
		timeRendering = m_StoppWatch.elapsed();
		waitTimeRendering = waitTimeRendering + (timeRendering - waitTimeRendering + 12)/4;
		LOG(lvlDebug, QString("DIRECT %1, time %2 wait %3").arg(id).arg(timeRendering).arg(waitTimeRendering));

	}

};