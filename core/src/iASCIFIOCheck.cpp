#include "iASCIFIOCheck.h"

#include "iAConsole.h"

#include <QCoreApplication>
#include <QFileInfo>

void CheckSCIFIO()
{
#ifdef USE_SCIFIO
	// Workaround for ITK requiring SCIFIO_PATH to be set when compiled with SCIFIO
	const char* SCIFIO_PATH = "SCIFIO_PATH";
	QString envScifioPath(getenv(SCIFIO_PATH));

	if (envScifioPath.length() > 0)
	{
		return;
	}

	QFileInfo fi(QCoreApplication::applicationDirPath());
	QString scifioPath(fi.absolutePath() + "/scifio_jars");
	if (!QFile::exists(scifioPath))
	{
		DEBUG_LOG(QString("ITK was built with SCIFIO, SCIFIO_PATH environment variable is not set, and scifio_jars directory (%1) was not found."
			"You might not be able to load files!").arg(scifioPath));
		return;
	}
	scifioPath.replace("/", "\\");
	QString scifioPathAssign(QString(SCIFIO_PATH) + "=" + scifioPath);
#ifdef _MSC_VER
	_putenv(scifioPathAssign.toStdString().c_str());
#else
	static char * scifioPathBuffer = new char[scifioPathAssign.length() + 1];
	strcpy(scifioPathBuffer, scifioPathAssign.toStdString().c_str());
	putenv(scifioPathBuffer);
#endif

#endif
}
