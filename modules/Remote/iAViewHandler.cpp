// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAViewHandler.h"

#include <iALog.h>

iAViewHandler::iAViewHandler(QString const & id): m_id(id)
{
	m_timer.setSingleShot(true);
	connect(&m_timer, &QTimer::timeout, [=]() -> void
	{
		//LOG(lvlDebug, "TIMER");
		emit createImage(m_id, 100);
	});
	m_stopWatch.start();
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
	
	if ((m_stopWatch.elapsed() > std::max(m_waitTimeRendering, MinWaitTime)))
	{
		m_stopWatch.restart();
		m_timer.stop();
		m_timer.start(FinalUpdateTime);

		emit createImage(m_id, ReducedQuality);
		m_timeRendering = m_stopWatch.elapsed();
		m_waitTimeRendering = m_waitTimeRendering + (m_timeRendering - m_waitTimeRendering + 12)/4;  // magic numbers -> gradual adaptation
		LOG(lvlDebug, QString("DIRECT %1, time %2 ms; wait %3 ms").arg(m_id).arg(m_timeRendering).arg(m_waitTimeRendering));
	}
	else
	{
		LOG(lvlDebug, "vtk callback ignored!");
	}
}
