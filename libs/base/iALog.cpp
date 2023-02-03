// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALog.h"
#include "iALogLevelMappings.h"

#include <QStringList>

iALogger* iALog::m_globalLogger(nullptr);

void iALog::setLogger(iALogger* logger)
{
	m_globalLogger = logger;
}

iALogger* iALog::get()
{
	return m_globalLogger;
}

// iALogger - move to separate iALogger.cpp?

QStringList AvailableLogLevels()
{
	static QStringList logLevelStrings;
	if (logLevelStrings.isEmpty())
	{
		logLevelStrings << "DEBUG" << "INFO " << "WARN " << "ERROR" << "FATAL";
	}
	return logLevelStrings;
}


QString logLevelToString(iALogLevel lvl)
{
	if (lvl < lvlDebug || lvl > lvlFatal)
	{
		return "?????";
	}
	return AvailableLogLevels()[lvl - 1];
}

iALogLevel stringToLogLevel(QString const& str, bool& ok)
{
	ok = true;
	for (int l = 0; l < AvailableLogLevels().size(); ++l)
	{
		if (str == AvailableLogLevels()[l])
		{
			return static_cast<iALogLevel>(l+1);
		}
	}
	ok = false;
	return lvlInfo;
}

iALogger::iALogger():
	m_logLevel(lvlInfo)
{}

iALogger::~iALogger()
{}

void iALogger::setLogLevel(iALogLevel level)
{
	m_logLevel = level;
}

iALogLevel iALogger::logLevel() const
{
	return m_logLevel;
}

void LOG(iALogLevel level, QString const& msg)
{
	if (iALog::get())
	{
		iALog::get()->log(level, msg);
	}
	// else    // if no logger set, discard? ToDo: use multiple separate loggers
}
