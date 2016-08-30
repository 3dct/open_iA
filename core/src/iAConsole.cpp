/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
		std::ofstream logfile("debug.log", std::ofstream::out | std::ofstream::app);
		logfile << QString("%1 %2\n")
			.arg(QLocale().toString(
				QDateTime::currentDateTime(),
				QLocale::ShortFormat))
			.arg(text)
			.toStdString();
		logfile.close();
	}
}

void iAConsole::SetLogToFile(bool value)
{
	m_logToFile = value;
}

bool iAConsole::IsLogToFileOn()
{
	return m_logToFile;
}

iAConsole::iAConsole() :
	m_console(new dlg_console()),
	m_logToFile(false),
	m_closed(false)
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
