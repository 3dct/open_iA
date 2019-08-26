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
#include "iAConsole.h"

#include "dlg_console.h"
#include "iARedirectVtkOutput.h"
#include "iARedirectItkOutput.h"
#include "io/iAFileUtils.h"

#include <QDateTime>

#include <fstream>


// iAGlobalLogger

iALogger* iAGlobalLogger::m_globalLogger(nullptr);


void iAGlobalLogger::setLogger(iALogger* logger)
{
	m_globalLogger = logger;
}
iALogger* iAGlobalLogger::get()

{
	return m_globalLogger;
}


// iAConsole

void iAConsole::log(QString const & text)
{
	emit logSignal(text);
}

void iAConsole::logSlot(QString const & text)
{
	// The log window prevents the whole application from shutting down
	// if it is still open at the time the program should exit.
	// Therefore, we don't reopen the console after the close() method
	// has been called. This allows the program to exit properly.
	if (!m_closed)
	{
		m_console->show();
		m_console->log(text);
	}
	if (m_logToFile)
	{
		std::ofstream logfile( getLocalEncodingFileName(m_logFileName).c_str(), std::ofstream::out | std::ofstream::app);
		logfile << QString("%1 %2\n")
			.arg(QLocale().toString(
				QDateTime::currentDateTime(),
				QLocale::ShortFormat))
			.arg(text)
			.toStdString();
		logfile.flush();
		logfile.close();
		if (logfile.bad())
		{
			m_console->log(QString("Could not write to logfile '%1', file output will be disabled for now.").arg(m_logFileName));
			m_fileLogError = true;
			m_logToFile = false;
		}
		else
		{
			m_fileLogError = false;
		}
	}
}

void iAConsole::setLogToFile(bool value, QString const & fileName, bool verbose)
{
	if (verbose && m_logToFile != value)
	{
		logSlot(QString("%1 logging to file '%2'...").arg(value ? "Enabling" : "Disabling").arg(m_logFileName));
	}
	m_logToFile = value;
	m_logFileName = fileName;
}

bool iAConsole::isLogToFileOn() const
{
	return m_logToFile;
}


bool iAConsole::isFileLogError() const
{
	return m_fileLogError;
}

QString iAConsole::logFileName() const
{
	return m_logFileName;
}

iAConsole::iAConsole() :
	m_console(new dlg_console()),
	m_logToFile(false),
	m_closed(false),
	m_fileLogError(false),
	m_logFileName("debug.log")
{
	// redirect VTK and ITK output to console window:
	m_vtkOutputWindow = vtkSmartPointer<iARedirectVtkOutput>::New();
	m_itkOutputWindow = iARedirectItkOutput::New();
	vtkOutputWindow::SetInstance(m_vtkOutputWindow);
	itk::OutputWindow::SetInstance(m_itkOutputWindow);

	connect(this, SIGNAL(logSignal(QString const &)), this, SLOT(logSlot(QString const &)));
}

iAConsole::~iAConsole()
{
	delete m_console;
}

iAConsole* iAConsole::instance()
{
	static iAConsole s_instance;
	return &s_instance;
}


void iAConsole::close()
{
	m_closed = true;
	m_console->close();
}


void iAConsole::closeInstance()
{
	instance()->close();
}


// iALogger

iALogger::~iALogger()
{}



// iAConsoleLogger

void iAConsoleLogger::log(QString const & msg)
{
	iAConsole::instance()->log(msg);
}

iAConsoleLogger * iAConsoleLogger::get()
{
	static iAConsoleLogger GlobalConsoleLogger;
	return &GlobalConsoleLogger;
}

iAConsoleLogger::iAConsoleLogger()
{}



// iAStdOutLogger

void iAStdOutLogger::log(QString const & msg)
{
	std::cout << msg.toStdString() << std::endl;
}

iAStdOutLogger * iAStdOutLogger::get()
{
	static iAStdOutLogger GlobalStdOutLogger;
	return &GlobalStdOutLogger;
}

iAStdOutLogger::iAStdOutLogger()
{}