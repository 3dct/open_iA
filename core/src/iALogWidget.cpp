/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iALogWidget.h"

#include "dlg_console.h"
#include "iARedirectVtkOutput.h"
#include "iARedirectItkOutput.h"
#include "io/iAFileUtils.h"

#include <QDateTime>

#include <fstream>


void iALogWidget::log(iALogLevel lvl, QString const & text)
{
	emit logSignal(lvl, text);
}

void iALogWidget::logSlot(iALogLevel lvl, QString const & text)
{
	// The log window prevents the whole application from shutting down
	// if it is still open at the time the program should exit.
	// Therefore, we don't reopen the console after the close() method
	// has been called. This allows the program to exit properly.
	if (!m_closed)
	{
		if (!m_console->isVisible())
		{
			m_console->show();
			emit consoleVisibilityChanged(true);
		}
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
			if (!m_closed)
			{
				m_console->log(QString("Could not write to logfile '%1', file output will be disabled for now.").arg(m_logFileName));
			}
			m_fileLogError = true;
			m_logToFile = false;
		}
		else
		{
			m_fileLogError = false;
		}
	}
}

void iALogWidget::setVisible(bool visible)
{
	if (m_closed)
	{
		return;
	}
	m_console->setVisible(visible);
}

QDockWidget* iALogWidget::dockWidget()
{
	return m_console;
}

void iALogWidget::setLogToFile(bool value, QString const & fileName, bool verbose)
{
	if (verbose && m_logToFile != value)
	{
		logSlot(lvlInfo, QString("%1 logging to file '%2'...").arg(value ? "Enabling" : "Disabling").arg(m_logFileName));
	}
	m_logToFile = value;
	m_logFileName = fileName;
}

bool iALogWidget::isLogToFileOn() const
{
	return m_logToFile;
}

bool iALogWidget::isFileLogError() const
{
	return m_fileLogError;
}

bool iALogWidget::isVisible() const
{
	return m_console->isVisible();
}

QString iALogWidget::logFileName() const
{
	return m_logFileName;
}

iALogWidget::iALogWidget() :
	m_logFileName("debug.log"),
	m_console(new dlg_console()),
	m_logToFile(false),
	m_closed(false),
	m_fileLogError(false)
{
	// redirect VTK and ITK output to console window:
	m_vtkOutputWindow = vtkSmartPointer<iARedirectVtkOutput>::New();
	m_itkOutputWindow = iARedirectItkOutput::New();
	vtkOutputWindow::SetInstance(m_vtkOutputWindow);
	itk::OutputWindow::SetInstance(m_itkOutputWindow);

	connect(this, &iALogWidget::logSignal, this, &iALogWidget::logSlot);
	connect(m_console, &dlg_console::onClose, this, &iALogWidget::consoleClosed);
}

iALogWidget::~iALogWidget()
{
}

iALogWidget* iALogWidget::get()
{
	static iALogWidget s_instance;
	return &s_instance;
}

void iALogWidget::consoleClosed()
{
	emit consoleVisibilityChanged(false);
}

void iALogWidget::close()
{
	m_closed = true;
	m_console->close();
}

void iALogWidget::closeInstance()
{
	get()->close();
}
