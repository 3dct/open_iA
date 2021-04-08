/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

// base
#include "iAFileUtils.h"
#include "iALogLevelMappings.h"

// core
#include "iALogRedirectVTK.h"
#include "iALogRedirectITK.h"

#include <QDateTime>

#include <fstream>


void iALogWidget::log(iALogLevel lvl, QString const & text)
{
	if (lvl >= m_logLevel || lvl >= m_fileLogLevel)
	{
		emit logSignal(lvl, text);
	}
}

void iALogWidget::logSlot(int lvl, QString const & text)
{
	// The log window prevents the whole application from shutting down
	// if it is still open at the time the program should exit.
	// Therefore, we don't reopen the console after the close() method
	// has been called. This allows the program to exit properly.
	if (!m_closed && lvl >= m_logLevel)
	{
		if (!isVisible())
		{
			show();
			emit logVisibilityChanged(true);
		}
		QString msg = QString("%1 %2 %3")
			.arg(QLocale().toString(QTime::currentTime(), "hh:mm:ss"))
			.arg(logLevelToString(static_cast<iALogLevel>(lvl)).left(1))
			.arg(text);
		if (lvl == lvlError)
		{
			msg = "<span style=\"color:red\">" + msg + "</span>";
		}
		logTextEdit->append(msg);
	}
	if (m_logToFile && lvl >= m_fileLogLevel)
	{
		QString msg = QString("%1 %2 %3")
			.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(logLevelToString(static_cast<iALogLevel>(lvl)))
			.arg(text);
		std::ofstream logfile( getLocalEncodingFileName(m_logFileName).c_str(), std::ofstream::out | std::ofstream::app);
		logfile << msg.toStdString() << std::endl;
		logfile.flush();
		logfile.close();
		if (logfile.bad())
		{
			if (!m_closed)
			{
				logTextEdit->append(QString("Could not write to logfile '%1', file output will be disabled for now.").arg(m_logFileName));
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

void iALogWidget::setLogToFile(bool value, QString const & fileName, bool verbose)
{
	if (verbose && m_logToFile != value)
	{
		logSlot(lvlInfo, QString("%1 logging to file '%2'...").arg(value ? "Enabling" : "Disabling").arg(m_logFileName));
	}
	m_logToFile = value;
	m_logFileName = fileName;
}

void iALogWidget::setFileLogLevel(iALogLevel lvl)
{
	m_fileLogLevel = lvl;
}

iALogLevel iALogWidget::fileLogLevel() const
{
	return m_fileLogLevel;
}

bool iALogWidget::isLogToFileOn() const
{
	return m_logToFile;
}

bool iALogWidget::isFileLogError() const
{
	return m_fileLogError;
}

QString iALogWidget::logFileName() const
{
	return m_logFileName;
}

iALogWidget::iALogWidget() :
	m_logFileName("debug.log"),
	m_logToFile(false),
	m_closed(false),
	m_fileLogError(false)
{
	setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	// redirect VTK and ITK output to console window:
	m_redirectVTK = vtkSmartPointer<iALogRedirectVTK>::New();
	m_redirectITK = iALogRedirectITK::New();
	vtkOutputWindow::SetInstance(m_redirectVTK);
	itk::OutputWindow::SetInstance(m_redirectITK);
	cmbboxLogLevel->addItems(AvailableLogLevels());

	connect(pbClearLog, &QPushButton::clicked, this, &iALogWidget::clear);
	connect(cmbboxLogLevel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iALogWidget::setLogLevelSlot);
	connect(this, &iALogWidget::logSignal, this, &iALogWidget::logSlot);
}

iALogWidget::~iALogWidget()
{
}

iALogWidget* iALogWidget::get()
{
	static iALogWidget* instance(new iALogWidget);
	return instance;
}

void iALogWidget::shutdown()
{
	get()->m_closed = true;
	get()->close();
}

void iALogWidget::clear()
{
	logTextEdit->clear();
}

void iALogWidget::closeEvent(QCloseEvent* event)
{
	emit logVisibilityChanged(false);
	QDockWidget::closeEvent(event);
}

void iALogWidget::setLogLevel(iALogLevel lvl)
{
	QSignalBlocker sb(cmbboxLogLevel);
	cmbboxLogLevel->setCurrentIndex(lvl - 1);
	iALogger::setLogLevel(lvl);
}

void iALogWidget::setLogLevelSlot(int selectedIdx)
{
	iALogger::setLogLevel(static_cast<iALogLevel>(selectedIdx + 1));
}
