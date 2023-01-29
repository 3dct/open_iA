// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

class QString;

//! Severity of log entries.
//! Values persisted to preferences (see mainwindow)
enum iALogLevel
{
	//! Very detailed information, only useful if trying to find a problem
	//! in a specific part of the code
	lvlDebug = 1,

	//! informational, might help users understand what is going on (example:
	//! which step of an operation is currently performed)
	lvlInfo  = 2,

	//!< Something the program didn't necessarily expect, and might have side
	//! effects the user should be aware of (example: some parameter was
	//! outside of its valid range and was reset to its default value)
	lvlWarn  = 3,
	
	//! A problematic situation the user should be aware of, and which
	//! probably aborts the current operation, but the program can keep on
	//! running (example: file could not be opened)
	lvlError = 4,
	
	//! A situation that requires the program to stop (example: no more memory
	//! available)
	lvlFatal = 5,

	//! Information that should be printed regardless of the log level
	lvlImportant = lvlFatal
};

//! Base interface for logging
//! implementation in iALog.cpp
class iAbase_API iALogger
{
public:
	iALogger();
	virtual ~iALogger();
	//! Log a given message with the given log level. The actual implementations
	//! (classes derived from iALogger) determine what happens with this information.
	//! @param level the severity of the message, see iALogLevel.
	//! @param msg the message to be logged.
	virtual void log(iALogLevel level, QString const & msg) =0;
	//! Sets the current loglevel of this logger.
	//! Only messages with a log level equal to or higher than the given level
	//! should be reported (loggers need to implement this based on the m_logLevel
	//! member below).
	virtual void setLogLevel(iALogLevel level);
	//! Retrieve current log level.
	iALogLevel logLevel() const;
protected:
	iALogLevel m_logLevel;
};
