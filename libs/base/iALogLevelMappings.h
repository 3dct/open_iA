// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iALog.h"

#include <QStringList>

class QString;

iAbase_API QString logLevelToString(iALogLevel lvl);
iAbase_API iALogLevel stringToLogLevel(QString const& str, bool& ok);
iAbase_API QStringList AvailableLogLevels();

// implementation in iALog.cpp
