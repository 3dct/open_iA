/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAPerformanceHelper.h"

#include "iAConsole.h"

#include <QString>

#include <iomanip>

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <cstdio>

#endif

#else
#error "Cannot define getCurrentRSS( ) for an unknown OS."
#endif


/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS( )
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
	/* OSX ------------------------------------------------------ */
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
		(task_info_t)&info, &infoCount ) != KERN_SUCCESS )
		return (size_t)0L;      /* Can't access? */
	return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	/* Linux ---------------------------------------------------- */
	long rss = 0L;
	FILE* fp = NULL;
	if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
		return (size_t)0L;      /* Can't open? */
	if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
	{
		fclose( fp );
		return (size_t)0L;      /* Can't read? */
	}
	fclose( fp );
	return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);

#else
	/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
	return (size_t)0L;          /* Unsupported. */
#endif
}

// class iAPerformanceTimer

class iAPerfTimerImpl
{
public:
	iAPerfTimerImpl() : m_start(std::chrono::system_clock::now()) {}
	std::chrono::system_clock::time_point m_start;
};

const double iAPerformanceTimer::DurationSecondFactor = 1000000.0;

iAPerformanceTimer::iAPerformanceTimer():
	m_pImpl(new iAPerfTimerImpl())
{}

iAPerformanceTimer::~iAPerformanceTimer()
{
	delete m_pImpl;
}

void iAPerformanceTimer::start()
{
	m_pImpl->m_start = std::chrono::system_clock::now();
}

iAPerformanceTimer::DurationType iAPerformanceTimer::elapsed() const
{
	std::chrono::system_clock::time_point lap = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(lap - m_pImpl->m_start).count() / DurationSecondFactor;
}

// class iATimeAdder

iATimeAdder::iATimeAdder():
	m_elapsed(0)
{}

void iATimeAdder::resume()
{
	m_timer.start();
}
void iATimeAdder::pause()
{
	m_elapsed += m_timer.elapsed();
}
iAPerformanceTimer::DurationType iATimeAdder::elapsed() const
{
	return m_elapsed;
}


// class iAPerformanceHelper
class iAPerfHelperImpl
{
public:
	iAPerfHelperImpl():
		m_caption(""),
		m_printMemUsage(false)
	{}
	iAPerformanceTimer  m_perfTimer;    //!< used for measuring the time
	std::string         m_caption;       //!< the name of the current operation
	bool                m_printMemUsage; //!< whether to print memory usage
};


iAPerformanceHelper::iAPerformanceHelper():
	m_pImpl(new iAPerfHelperImpl())
{}

iAPerformanceHelper::~iAPerformanceHelper()
{
	delete m_pImpl;
}

void iAPerformanceHelper::printTime(iAPerformanceTimer::DurationType duration, std::string const & caption, bool printMemUsage)
{
	DEBUG_LOG(QString("%1 %2%3")
		.arg(caption.c_str())
		.arg(formatDuration(duration))
		.arg(printMemUsage ? printMemoryUsage() : "")
	);
}

QString iAPerformanceHelper::printMemoryUsage()
{
	return QString("; memory usage: %1  MB").arg(getCurrentRSS()/1048576);
}

void iAPerformanceHelper::start(std::string const & caption, bool printMemUsage)
{
	m_pImpl->m_caption = caption;
	m_pImpl->m_printMemUsage = printMemUsage;
	m_pImpl->m_perfTimer.start();
	DEBUG_LOG(QString(">>>>> START %1 %2")
		.arg(m_pImpl->m_caption.c_str())
		.arg(m_pImpl->m_printMemUsage ? printMemoryUsage(): "")
	);
}



iAPerformanceTimer::DurationType iAPerformanceHelper::time(std::string const & caption) const
{
	iAPerformanceTimer::DurationType duration = m_pImpl->m_perfTimer.elapsed();
	printTime(duration, std::string("----- ") + caption + ": ", m_pImpl->m_printMemUsage);
	return duration;
}

iAPerformanceTimer::DurationType iAPerformanceHelper::stop()
{
	iAPerformanceTimer::DurationType duration = m_pImpl->m_perfTimer.elapsed();
	printTime(duration, std::string("<<<<< END ") + m_pImpl->m_caption + ": ", m_pImpl->m_printMemUsage);
	return duration;
}


// class iATimeGuard

iATimeGuard::iATimeGuard(std::string const & caption, bool printMemUsage)
{
	m_perfHelper.start(caption, printMemUsage);
}

void iATimeGuard::time(std::string const & caption)
{
	m_perfHelper.time(caption);
}

iATimeGuard::~iATimeGuard()
{
	m_perfHelper.stop();
}

QString formatDuration(double duration)
{
	long secondsLong = static_cast<long>(duration);
	long secondPart = secondsLong % 60;
	long milliSeconds = (duration - secondsLong) * 1000;
	QString result;
	if (secondsLong < 10)
	{
		result = QString::number(milliSeconds)+QString(" ms");
	}
	result = QString::number(secondPart)+QString(" seconds ")+result;
	if (secondsLong >= 60)
	{
		long minutesLong = secondsLong / 60;
		long minutesPart = minutesLong % 60;
		result = QString::number(minutesPart)+QString(" minutes ")+result;
		if (minutesLong >= 60)
		{
			long hoursLong = minutesLong / 60;
			long hoursPart = hoursLong % 24;
			result = QString::number(hoursPart)+QString(" hours ")+result;
			if (hoursLong >= 24)
			{
				long daysLong = hoursLong / 24;
				result = QString::number(daysLong)+QString(" days ")+result;
			}
		}
	}
	return result;
}