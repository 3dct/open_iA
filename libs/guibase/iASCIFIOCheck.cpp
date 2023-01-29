// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASCIFIOCheck.h"

#include "iALog.h"

#include <QFileInfo>

void CheckSCIFIO(QString const &
#ifdef USE_SCIFIO
	applicationPath
#endif
)
{
#ifdef USE_SCIFIO
	// Workaround for ITK requiring SCIFIO_PATH to be set when compiled with SCIFIO
	const char* SCIFIO_PATH = "SCIFIO_PATH";
	QString envScifioPath(getenv(SCIFIO_PATH));

	if (envScifioPath.length() > 0)
	{
		return;
	}

	QFileInfo fi(applicationPath);
	QString scifioPath(fi.absoluteFilePath() + "/scifio_jars");
	if (!QFile::exists(scifioPath))
	{
		LOG(lvlWarn, QString("ITK was built with SCIFIO, SCIFIO_PATH environment variable is not set, and scifio_jars directory (%1) was not found."
			"You might not be able to load files!").arg(scifioPath));
		return;
	}
#ifdef WIN32
	scifioPath.replace("/", "\\");
#endif
	QString scifioPathAssign(QString(SCIFIO_PATH) + "=" + scifioPath);
#ifdef _MSC_VER
	_putenv(scifioPathAssign.toStdString().c_str()); // TODO: use _wputenv?
#else
	static char * scifioPathBuffer = new char[scifioPathAssign.length() + 1];
	strcpy(scifioPathBuffer, scifioPathAssign.toStdString().c_str());
	putenv(scifioPathBuffer);
#endif

#endif
}
