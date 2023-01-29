// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

class iACommandLineProgressIndicator : public QObject
{
	Q_OBJECT
public:
	iACommandLineProgressIndicator(int numberOfSteps, bool quiet);
public slots:
	void Progress(int percent);
private:
	int m_lastDots;
	int m_numberOfDots;
	bool m_quiet;
};

int ProcessCommandLine(int argc, char const * const * argv, const char * version);
