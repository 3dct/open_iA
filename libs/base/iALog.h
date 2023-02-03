// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
