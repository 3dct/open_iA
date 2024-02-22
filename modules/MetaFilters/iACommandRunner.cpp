// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACommandRunner.h"

#include <iALog.h>

#include <QFileInfo>

iACommandRunner::iACommandRunner(QString const & executable, QStringList const & arguments) :
	m_executable(executable),
	m_arguments(arguments)
{
}

void iACommandRunner::performWork()
{
	QProcess myProcess;
	myProcess.setProgram(m_executable);
	QFileInfo fi(m_executable);
	myProcess.setWorkingDirectory(fi.absolutePath());
	myProcess.setArguments(m_arguments);
	LOG(lvlInfo, QString("Running '%1' with arguments '%2'").arg(m_executable).arg(m_arguments.join(" ")));
	myProcess.setProcessChannelMode(QProcess::MergedChannels);
	connect(&myProcess, &QProcess::errorOccurred, this, &iACommandRunner::errorOccured);
	myProcess.start();
	myProcess.waitForFinished(-1);
	if (myProcess.exitStatus() != QProcess::NormalExit)
	{
		LOG(lvlError, "Program crashed!");
	}
	else
	{
		int statusCode = myProcess.exitCode();
		setSuccess(statusCode == 0);
		if (!success())
		{
			LOG(lvlError, QString("Program exited with status code %1").arg(myProcess.exitCode()));
		}
	}
	m_output = myProcess.readAllStandardOutput();
	m_output.replace("\r", "");
}

void iACommandRunner::errorOccured(QProcess::ProcessError p)
{
	LOG(lvlError, QString("CommandRunner: An error has occured %1").arg
	(p == QProcess::FailedToStart ? "failed to start" :
		p == QProcess::Crashed ? "Crashed" :
		p == QProcess::Timedout ? "Timedout" :
		p == QProcess::ReadError ? "Read Error" :
		p == QProcess::WriteError ? "Write Error" :
		"Unknown Error"));
}

QString iACommandRunner::output() const
{
	return m_output;
}
