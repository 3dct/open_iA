/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
 
#include "pch.h"
#include "iAConsole.h"

#include "dlg_console.h"
#include "iARedirectVtkOutput.h"

#include <QDateTime>

#include <fstream>

void iAConsole::Log(std::string const & text)
{
	Log(QString::fromStdString(text));
}

void iAConsole::Log(char const * text)
{
	Log(QString(text));
}

void iAConsole::Log(QString const & text)
{
	emit LogSignal(text);
}

void iAConsole::LogSlot(QString const & text)
{
	// The log window prevents the whole application from shutting down
	// if it is still open at the time the program should exit.
	// Therefore, we don't reopen the console after the close() method
	// has been called. This allows the program to exit properly.
	if (!m_closed)
	{
		m_console->show();
		m_console->Log(text);
	}
	if (m_logToFile)
	{
		std::ofstream logfile(m_logFileName.toStdString().c_str(), std::ofstream::out | std::ofstream::app);
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
			m_console->Log(QString("Could not write to logfile '%1', file output will be disabled for now.").arg(m_logFileName));
			m_fileLogError = true;
			m_logToFile = false;
		}
		else
		{
			m_fileLogError = false;
		}
	}
}

void iAConsole::SetLogToFile(bool value, QString const & fileName, bool verbose)
{
	if (verbose && m_logToFile != value)
	{
		LogSlot(QString("%1 logging to file '%2'...").arg(value ? "Enabling" : "Disabling").arg(m_logFileName));
	}
	m_logToFile = value;
	m_logFileName = fileName;
}

bool iAConsole::IsLogToFileOn() const
{
	return m_logToFile;
}


bool iAConsole::IsFileLogError() const
{
	return m_fileLogError;
}

QString iAConsole::GetLogFileName() const
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
	// redirect VTK output to console window:
	m_vtkOutputWindow = vtkSmartPointer<iARedirectVtkOutput>::New();
	vtkOutputWindow::SetInstance(m_vtkOutputWindow);

	connect(this, SIGNAL(LogSignal(QString const &)), this, SLOT(LogSlot(QString const &)));
}

iAConsole::~iAConsole()
{
	delete m_console;
}

iAConsole& iAConsole::GetInstance()
{
	static iAConsole instance;
	return instance;
}


void iAConsole::close()
{
	m_closed = true;
	m_console->close();
}


void iAConsole::Close()
{
	GetInstance().close();
}



// iAConsoleLogger

void iAConsoleLogger::log(QString const & msg)
{
	iAConsole::GetInstance().Log(msg.toStdString());
}

iAConsoleLogger & iAConsoleLogger::Get()
{
	static iAConsoleLogger GlobalConsoleLogger;
	return GlobalConsoleLogger;
}

iAConsoleLogger::iAConsoleLogger()
{}
