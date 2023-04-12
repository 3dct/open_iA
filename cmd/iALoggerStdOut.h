// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iALogger.h"

#include <QObject>  // for Q_DISABLE_COPY_MOVE

//! A logger writing its output to standard output.
class iALoggerStdOut : public iALogger
{
public:
	void log(iALogLevel lvl, QString const& msg) override;
	static iALoggerStdOut* get();
private:
	//! make default constructor private
	iALoggerStdOut() =default;
	Q_DISABLE_COPY_MOVE(iALoggerStdOut);
};
