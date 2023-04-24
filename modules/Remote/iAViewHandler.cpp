// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAViewHandler.h"

#include <iALog.h>

iAViewHandler::iAViewHandler()
{
	timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, &QTimer::timeout, [=]() -> void {
		//LOG(lvlDebug, "TIMER");
		createImage(id, 100);
	});
	m_StoppWatch.start();
}

void iAViewHandler::vtkCallbackFunc(vtkObject* caller, long unsigned int evId, void* callData)
{
	Q_UNUSED(caller);
	Q_UNUSED(evId);
	Q_UNUSED(callData);
	//LOG(lvlDebug, QString("DIRECT time check %1, time %2").arg(id).arg(m_StoppWatch.elapsed()));

	if ((m_StoppWatch.elapsed() > waitTimeRendering) && (m_StoppWatch.elapsed() >50))
	{
		m_StoppWatch.restart();
		timer->stop();
		timer->start(250);

		createImage(id, quality);
		timeRendering = m_StoppWatch.elapsed();
		waitTimeRendering = waitTimeRendering + (timeRendering - waitTimeRendering + 12)/4;
		//LOG(lvlDebug, QString("DIRECT %1, time %2 wait %3").arg(id).arg(timeRendering).arg(waitTimeRendering));
	}
}
