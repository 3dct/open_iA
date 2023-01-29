// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACommandLineProcessor.h"
#include "iALog.h"
#include "iALoggerStdOut.h"
#include "iASCIFIOCheck.h"
#include "iASettings.h"    // for initializeSettingsTypes
#include "version.h"

#include <QFileInfo>

int main(int argc, char *argv[])
{
	iALog::setLogger(iALoggerStdOut::get());
	QFileInfo fi(argv[0]);
	CheckSCIFIO(fi.absolutePath());
	initializeSettingTypes();
	return ProcessCommandLine(argc, argv, Open_iA_Version);
}
