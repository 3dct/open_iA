// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

//! A progress indicator for the command line, printing to standard out.
//! Prints end markers and dots marking the current progress, such as:
//!     |----------|
//!      .....
//! for a progress of 50%
class iACommandLineProgressIndicator : public QObject
{
	Q_OBJECT
public:
	iACommandLineProgressIndicator(int numberOfSteps, bool quiet);
public slots:
	void progress(int percent);
private:
	int m_lastDots;
	int m_numberOfDots;
	bool m_quiet;
};

int processCommandLine(int argc, char const * const * argv, const char * version);
