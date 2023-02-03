// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASampleOperation.h"

#include <iALog.h>

iASampleOperation::iASampleOperation():
	m_success(false)
{}

iASampleOperation::~iASampleOperation()
{}

void iASampleOperation::run()
{
	m_timer.start();
	try
	{
		performWork();
	}
	catch (std::exception& e)
	{
		LOG(lvlError, QString("iASampleOperation: An exception has occurred: %1").arg(e.what()));
		setSuccess(false);
	}
	m_duration = m_timer.elapsed();
}

iAPerformanceTimer::DurationType iASampleOperation::duration() const
{
	return m_duration;
}

bool iASampleOperation::success() const
{
	return m_success;
}

void iASampleOperation::setSuccess(bool success)
{
	m_success = success;
}
