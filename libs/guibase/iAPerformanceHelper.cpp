// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPerformanceHelper.h"

#include "iALog.h"

#include <QString>

#include <chrono>

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
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


//! Returns the current resident set size (physical memory use) measured
//! in bytes, or zero if the value cannot be determined on this OS.
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
	FILE* fp = nullptr;
	if ( (fp = fopen( "/proc/self/statm", "r" )) == nullptr )
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
	LOG(lvlInfo, QString("%1 %2%3")
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
	LOG(lvlInfo, QString(">>>>> START %1 %2")
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

iAPerformanceTimer::DurationType iAPerformanceHelper::elapsed() const
{
	return m_pImpl->m_perfTimer.elapsed();
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

iAPerformanceTimer::DurationType iATimeGuard::elapsed() const
{
	return m_perfHelper.elapsed();
}

QString formatWithCaption(long part, QString const& caption, QString const& shortCap, bool useShort)
{
	return QString::number(part) + " " + (useShort ? shortCap : caption + ((part != 1) ? "s" : "")) + " ";
}

QString formatDuration(double duration, bool showMS, bool shortNames)
{
	long secondsLong = static_cast<long>(duration);
	long secondPart = secondsLong % 60;
	long milliSeconds = (duration - secondsLong) * 1000;
	QString result;
	if (showMS && secondsLong < 10)
	{
		result = QString::number(milliSeconds)+QString(" ms");
	}
	result = formatWithCaption(secondPart, "second", "s", shortNames) + result;
	if (secondsLong >= 60)
	{
		long minutesLong = secondsLong / 60;
		long minutesPart = minutesLong % 60;
		result = formatWithCaption(minutesPart, "minute", "m", shortNames) + result;
		if (minutesLong >= 60)
		{
			long hoursLong = minutesLong / 60;
			long hoursPart = hoursLong % 24;
			result = formatWithCaption(hoursPart, "hour", "h", shortNames) + result;
			if (hoursLong >= 24)
			{
				long daysLong = hoursLong / 24;
				result = formatWithCaption(daysLong, "day", "d", shortNames) + result;
			}
		}
	}
	return result;
}
