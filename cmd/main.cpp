// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACommandLineProcessor.h"
#include "iALog.h"
#include "iALoggerStdOut.h"
#include "iASCIFIOCheck.h"
#include "version.h"

#include <QFileInfo>

int main(int argc, char *argv[])
{
	iALog::setLogger(iALoggerStdOut::get());
	QFileInfo fi(argv[0]);
	CheckSCIFIO(fi.absolutePath());
	return processCommandLine(argc, argv, Open_iA_Version);
}
