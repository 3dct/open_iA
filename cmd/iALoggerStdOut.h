// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iALogger.h"

//! A logger whose output is written to standard output.
class iALoggerStdOut : public iALogger
{
public:
	void log(iALogLevel lvl, QString const& msg) override;
	static iALoggerStdOut* get();
private:
	//! make default constructor private
	iALoggerStdOut() =default;
	//! @{ don't allow copying / copy-assingment
	iALoggerStdOut(iALoggerStdOut const&) =delete;
	void operator=(iALoggerStdOut const&) =delete;
	//! @}
};
