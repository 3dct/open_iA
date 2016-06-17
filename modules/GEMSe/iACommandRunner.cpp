#include "iACommandRunner.h"

#include "iAConsole.h"

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
	myProcess.setArguments(m_arguments);
	DEBUG_LOG(QString("Running '%1' with arguments '%2'\n").arg(m_executable).arg(m_arguments.join(" ")));
	myProcess.setProcessChannelMode(QProcess::MergedChannels);
	connect(&myProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(errorOccured(QProcess::ProcessError)));
	myProcess.start();
	myProcess.waitForFinished();
	m_output = myProcess.readAllStandardOutput().toStdString();
	m_duration = m_timer.elapsed();
}


void iACommandRunner::errorOccured(QProcess::ProcessError p)
{
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