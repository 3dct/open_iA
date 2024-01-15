// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALoggerStdOut.h"

#include "iALogLevelMappings.h"

#include <iostream>

#include <QString>

void iALoggerStdOut::log(iALogLevel lvl, QString const& msg)
{
	if (lvl < m_logLevel)
	{
		return;
	}
	std::cout << logLevelToString(lvl).toStdString() << ": " << msg.toStdString() << std::endl;
}

iALoggerStdOut* iALoggerStdOut::get()
{
	static iALoggerStdOut GlobalStdOutLogger;
	return &GlobalStdOutLogger;
}
