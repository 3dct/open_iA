/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#ifdef NO_DLL_LINKAGE
// Log output support in tests
#include <iostream>
#define LOG(lvlInfo, msg) std::cout << msg.toStdString() << std::endl;
#else

#include "iALogger.h"

#include <QString>    // do not remove! required for linux build!

//! Singleton providing access to the global logger object.
//! Before first access (via get()), a specific logger needs to be set
//! via setLogger(...). See classes derived from iALogger
class iAbase_API iALog
{
public:
    //! Set the class to perform the actual logging
    //! @param logger a concrete implementation of the iALogger interface
    static void setLogger(iALogger* logger);
    //! Retrieve the global logger implementation
    static iALogger* get();
private:
    static iALogger* m_globalLogger;
    //! @{ Prevent copy construction and assignment
    iALog(iALog const&) = delete;
    void operator=(iALog const&) = delete;
    //! @}
};

iAbase_API void LOG(iALogLevel level, QString const& msg);

#endif
