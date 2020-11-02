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
#pragma once

#include "open_iA_Core_export.h"

#include <QString>

//! Severity of log entries.
//! Implementation note: actual values currently not important
//! (as they are not persisted anywhere).
enum iALogLevel
{
	//! Extremely detailed information, only useful if trying to find a problem
	//! in a specific part of the code
	lvlDebug = 10,

	//! informational, might help users understand what is going on (example:
	//! which step of an operation is currently performed)
	lvlInfo  = 20,

	//!< Something the program didn't necessarily expect, and might have side
	//! effects the user should be aware of (example: some parameter was
	//! outside of its valid range and was reset to its default value)
	lvlWarn  = 30,
	
	//! A problematic situation the user should be aware of, and which
	//! probably aborts the current operation, but the program can keep on
	//! running (example: file could not be opened)
	lvlError = 40,
	
	//! A situation that requires the program to stop (example: no more memory
	//! available)
	lvlFatal = 50
};

open_iA_Core_API QString logLevelToString(iALogLevel lvl);

//! Base interface for logging
class open_iA_Core_API iALogger
{
public:
	virtual ~iALogger();
	//! Log a given message with the given log level. The actual implementations
	//! (classes derived from iALogger) determine what happens with this information.
	//! @param level the severity of the message, see iALogLevel.
	//! @param msg the message to be logged.
	virtual void log(iALogLevel level, QString const & msg) =0;
};
