/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "open_iA_Core_export.h"

#include <chrono>
#include <string>

class QString;

class iAPerfTimerImpl;

//! Class for simple performance measurements.
//! Holds a reference start time and allows to retrieve the time elapsed since
//! that time
class open_iA_Core_API iAPerformanceTimer
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


//! class for adding up intervals of time
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
class open_iA_Core_API iATimeAdder
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

//! Class for debug output about start and end of an operation
//! call start() to print start message
//! call stop() to print stop message + time
//! call time() in between to show lap times
//! all optionally including memory usage
class open_iA_Core_API iAPerformanceHelper
{
public:
	iAPerformanceHelper();
	~iAPerformanceHelper();
	void start(std::string const & caption = "", bool printMemUsage = true);
	iAPerformanceTimer::DurationType time(std::string const & caption = "") const;
	iAPerformanceTimer::DurationType stop();

	//! helper method for printing the time (and optionally memory usage)
	static void printTime(iAPerformanceTimer::DurationType, std::string const & caption = "", bool printMemUsage = false);
	//! print memory usage information
	static QString printMemoryUsage();
private:
	iAPerfHelperImpl*     m_pImpl;
};

//! Simple performance helper class following RAII principle:
//! Instantiate to start timer, destroy to stop timer
//! prints to debug console
class open_iA_Core_API iATimeGuard
{
public:
	//! starts measuring and writes according message (and optionally memory
	//! usage) to the debug console
	iATimeGuard(std::string const & caption = "", bool printMemUsage = true);
	//! output an intermediate time with an optional caption
	void time(std::string const & caption = "");
	//! destructor, stops timer and outputs duration (and optionally memory
	//! usage) to the debug console
	~iATimeGuard();
private:
	iAPerformanceHelper m_perfHelper;
};

//! Helper method for getting the current memory usage
//! @return the number of bytes currently in use by the application
size_t getCurrentRSS();

//! format the given time in a human-readable format
//! @param duration the time to format (in seconds)
open_iA_Core_API QString formatDuration(double duration);
