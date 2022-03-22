/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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