#pragma once


#include "iAPerformanceHelper.h"

#include <QProcess>
#include <QThread>

class iACommandRunner : public QThread
{
	Q_OBJECT
public:
	iACommandRunner(QString const & executable, QStringList const & arguments);
	void run();
	iAPerformanceTimer::DurationType duration() const;
	std::string output() const;
	bool success() const;
private slots:
	void errorOccured(QProcess::ProcessError);
private:
	QString m_executable;
	QStringList m_arguments;
	iAPerformanceTimer m_timer;
	iAPerformanceTimer::DurationType m_duration;
	std::string m_output;
	bool m_success;
}; 
