// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAViewHandler.h"

#include <iALog.h>

iAViewHandler::iAViewHandler()
{
	timer.setSingleShot(true);
	connect(&timer, &QTimer::timeout, [=]() -> void {
		//LOG(lvlDebug, "TIMER");
		emit createImage(id, 100);
	});
	m_StoppWatch.start();
}

void iAViewHandler::vtkCallbackFunc(vtkObject* caller, long unsigned int evId, void* callData)
{
	Q_UNUSED(caller);
	Q_UNUSED(evId);
	Q_UNUSED(callData);
	//LOG(lvlDebug, QString("DIRECT time check %1, time %2").arg(id).arg(m_StoppWatch.elapsed()));
	const int MinWaitTime = 50;
	const int FinalUpdateTime = 250;
	const int ReducedQuality = 20;   // reduce resolution as well?
	if ((m_StoppWatch.elapsed() > std::max(waitTimeRendering, MinWaitTime)))
	{
		m_StoppWatch.restart();
		timer.stop();
		timer.start(FinalUpdateTime);

		emit createImage(id, ReducedQuality);
		timeRendering = m_StoppWatch.elapsed();
		waitTimeRendering = waitTimeRendering + (timeRendering - waitTimeRendering + 12)/4;  // magic numbers -> gradual adaptation
		LOG(lvlDebug, QString("DIRECT %1, time %2 ms; wait %3 ms").arg(id).arg(timeRendering).arg(waitTimeRendering));
	}
	else
	{
		LOG(lvlDebug, "vtk callback ignored!");
	}
}
