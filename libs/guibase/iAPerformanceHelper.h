// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <string>

class QString;

class iAPerfTimerImpl;

//! Class for simple performance measurements.
//! Holds a reference start time and allows to retrieve the time elapsed since
//! that time
//! @deprecated use QElapsedTimer instead
class iAguibase_API iAPerformanceTimer
{
public:
	//! the time resolution type to use
	//typedef std::chrono::microseconds DurationType;
	typedef double DurationType;
	//! factor for how many counts are one second
	//! Has to match the DurationType!
	static const double DurationSecondFactor;
	//! Create a performance helper. Records the start time just as start()
	//! does
	iAPerformanceTimer();
	//! destructor
	~iAPerformanceTimer();
	//! Sets the reference start time. Only needed if measurement should start
	//! at a time after construction (as constructor already sets it), or if
	//! class is reused to time multiple events
	void start();
	//! retrieve the time elapsed since the reference start time
	DurationType elapsed() const;
private:
	iAPerfTimerImpl* m_pImpl;
};


//! Class for adding up intervals of time.
//! one example use case is if you have a long running procedure with many steps,
//! but only want to measure the contribution of some of the steps; e.g.:
//! do {
//!     operationsToMeasure (...)
//!     ... some other operations ...
//! }
//! Then you would write:
//! iATimeAdder timer;
//! do {
//!     timer.resume();
//!     operationsToMeasure (...)
//!     timer.pause();
//!     ... some other operations ...
//! }
//! std::cout << "Elapsed: " << timer.elapsed() << " seconds";
class iAguibase_API iATimeAdder
{
public:
	iATimeAdder();
	void resume();
	void pause();
	iAPerformanceTimer::DurationType elapsed() const;
private:
	iAPerformanceTimer::DurationType m_elapsed;
	iAPerformanceTimer m_timer;
};

class iAPerfHelperImpl;

//! Class for debug output about start and end of an operation.
//! call start() to print start message
//! call stop() to print stop message + time
//! call time() in between to show lap times
//! all optionally including memory usage
class iAguibase_API iAPerformanceHelper
{
public:
	iAPerformanceHelper();
	~iAPerformanceHelper();
	void start(std::string const & caption = "", bool printMemUsage = true);
	iAPerformanceTimer::DurationType time(std::string const & caption = "") const;
	iAPerformanceTimer::DurationType stop();
	iAPerformanceTimer::DurationType elapsed() const;

	//! helper method for printing the time (and optionally memory usage)
	static void printTime(iAPerformanceTimer::DurationType, std::string const & caption = "", bool printMemUsage = false);
	//! print memory usage information
	static QString printMemoryUsage();
private:
	iAPerfHelperImpl*     m_pImpl;
};

//! Simple performance helper class following RAII principle.
//! Instantiate to start timer, destroy to stop timer;
//! prints to log window
class iAguibase_API iATimeGuard
{
public:
	//! Starts measuring and writes according message (and optionally memory.
	//! usage) to the log window
	iATimeGuard(std::string const & caption = "", bool printMemUsage = true);
	//! output an intermediate time with an optional caption
	void time(std::string const & caption = "");
	iAPerformanceTimer::DurationType elapsed() const;
	//! destructor, stops timer and outputs duration (and optionally memory
	//! usage) to the log window
	~iATimeGuard();
private:
	iAPerformanceHelper m_perfHelper;
};

//! Helper method for getting the current memory usage.
//! @return the number of bytes currently in use by the application
size_t getCurrentRSS();

//! Format the given time in a human-readable format.
//! @param duration the time to format (in seconds)
//! @param showMS whether to show the milliseconds part
//! @param shortNames if true, use short time span names (s, m, h, d), otherwise (default), use long names (second, minute, hour, day)
iAguibase_API QString formatDuration(double duration, bool showMS = true, bool shortNames=false);
// TODO: refactor to use milliseconds input?
//iAguibase_API QString formatDurationSeconds(double duration, bool showMS = true, bool shortNames = false);
//iAguibase_API QString formatDurationMS(uint64_t ms, bool showMS = true, bool shortNames = false);
