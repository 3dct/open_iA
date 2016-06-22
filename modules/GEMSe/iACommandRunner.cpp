#include "iACommandRunner.h"

#include "iAConsole.h"

#include <QFileInfo>

iACommandRunner::iACommandRunner(QString const & executable, QStringList const & arguments)
	:m_executable(executable),
	m_arguments(arguments)
{
}


void iACommandRunner::run()
{
	m_timer.start();
	QProcess myProcess;
	myProcess.setProgram(m_executable);
	QFileInfo fi(m_executable);
	myProcess.setWorkingDirectory(fi.absolutePath());
	myProcess.setArguments(m_arguments);
	DEBUG_LOG(QString("Running '%1' with arguments '%2'\n").arg(m_executable).arg(m_arguments.join(" ")));
	myProcess.setProcessChannelMode(QProcess::MergedChannels);
	connect(&myProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(errorOccured(QProcess::ProcessError)));
	myProcess.start();
	myProcess.waitForFinished();
	if (myProcess.exitStatus() != QProcess::NormalExit)
	{
		m_success = false;
		DEBUG_LOG("Program crashed!\n");
	}
	else
	{
		DEBUG_LOG(QString("Program exited with status code %1\n").arg(myProcess.exitCode()));
		int statusCode = myProcess.exitCode();
		m_success = (statusCode == 0);
	}
	m_output = myProcess.readAllStandardOutput().toStdString();
	m_duration = m_timer.elapsed();
}


void iACommandRunner::errorOccured(QProcess::ProcessError p)
{
	DEBUG_LOG(QString("CommandRunner: An error has occured %1\n").arg
	(p == QProcess::FailedToStart ? "failed to start" :
		p == QProcess::Crashed ? "Crashed" :
		p == QProcess::Timedout ? "Timedout" :
		p == QProcess::ReadError ? "Read Error" :
		p == QProcess::WriteError ? "Write Error" :
		"Unknown Error"));
	m_success = false;
}

iAPerformanceTimer::DurationType iACommandRunner::duration() const
{
	return m_duration;
}


std::string iACommandRunner::output() const
{
	return m_output;
}


bool iACommandRunner::success() const
{
	return m_success;
}