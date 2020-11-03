/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iALog.h"
#include "iALogLevelMappings.h"

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
	if (lvl < lvlDebug && lvl > lvlFatal)
	{
		return "?????";
	}
	return AvailableLogLevels()[lvl - 1];
}

open_iA_Core_API iALogLevel stringToLogLevel(QString const& str, bool& ok)
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
	return lvlWarn;
}

iALogger::iALogger():
	m_logLevel(lvlWarn)
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
