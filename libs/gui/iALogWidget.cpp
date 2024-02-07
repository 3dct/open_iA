// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALogWidget.h"

// base
#include "iAFileUtils.h"
#include "iALogLevelMappings.h"

// core
#include "iALogRedirectVTK.h"
#include "iALogRedirectITK.h"

#include <QDateTime>

#include <cassert>
#include <fstream>

namespace
{
	QString formatMsg(iALogLevel lvl, QString const& text)
	{
		return QString("%1 %2 %3")
			.arg(QLocale().toString(QTime::currentTime(), "hh:mm:ss.zzz"))
			.arg(logLevelToString(static_cast<iALogLevel>(lvl)).left(1))
			.arg(text);
	}
}

void iALogWidget::log(iALogLevel lvl, QString const & text)
{
	if (lvl >= m_logLevel || lvl >= m_fileLogLevel)
	{
		emit logSignal(lvl, formatMsg(lvl, text));
	}
}

void iALogWidget::addText(QString const text)
{
	auto prevCursor = logTextEdit->textCursor();
	logTextEdit->moveCursor(QTextCursor::End);
	logTextEdit->insertPlainText(text+"\n");
	logTextEdit->setTextCursor(prevCursor);
}

void iALogWidget::logSlot(int lvl, QString const & text)
{
	// The log window prevents the whole application from shutting down
	// if it is still open at the time the program should exit.
	// Therefore, we don't reopen the console after the close() method
	// has been called. This allows the program to exit properly.
	if (!m_closed && lvl >= m_logLevel)
	{
		if (!isVisible() && m_openOnNewMessage)
		{
			show();
		}
		addText(text);
	}
	if (m_logToFile && lvl >= m_fileLogLevel)
	{
		std::ofstream logfile( getLocalEncodingFileName(m_logFileName).c_str(), std::ofstream::out | std::ofstream::app);
		logfile << text.toStdString() << std::endl;
		logfile.flush();
		logfile.close();
		if (logfile.bad())
		{
			if (!m_closed)
			{
				addText(formatMsg(static_cast<iALogLevel>(lvl), QString("Could not write to logfile '%1', file output will be disabled for now.").arg(m_logFileName)));
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

void iALogWidget::setLogToFile(bool enable, QString const & fileName, bool verbose)
{
	if (verbose && m_logToFile != enable)
	{
		logSlot(lvlInfo, QString("%1 logging to file '%2'...").arg(enable ? "Enabling" : "Disabling")
			.arg(enable ? fileName : m_logFileName));
	}
	m_logToFile = enable;
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
	m_fileLogError(false),
	m_openOnNewMessage(true)
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
	connect(cbVTK, &QCheckBox::stateChanged, this, &iALogWidget::toggleVTK);
	connect(cbITK, &QCheckBox::stateChanged, this, &iALogWidget::toggleITK);
	connect(this, &iALogWidget::logSignal, this, &iALogWidget::logSlot);
}

iALogWidget::~iALogWidget() = default;

iALogWidget* iALogWidget::get()
{
	static iALogWidget* instance(new iALogWidget);
	return instance;
}

void iALogWidget::shutdown()
{
	get()->m_closed = true;
	get()->close();
	iALog::setLogger(nullptr);
}

void iALogWidget::clear()
{
	logTextEdit->clear();
}

void iALogWidget::closeEvent(QCloseEvent* event)
{
	QDockWidget::closeEvent(event);
}

void iALogWidget::setLogLevel(iALogLevel lvl)
{
	QSignalBlocker sb(cmbboxLogLevel);
	cmbboxLogLevel->setCurrentIndex(lvl - 1);
	iALogger::setLogLevel(lvl);
}

void iALogWidget::setOpenOnNewMessage(bool openOnNewMessage)
{
	m_openOnNewMessage = openOnNewMessage;
}

void iALogWidget::setLogLevelSlot(int selectedIdx)
{
	iALogger::setLogLevel(static_cast<iALogLevel>(selectedIdx + 1));
}

void iALogWidget::toggleITK(int state)
{
	m_redirectITK->setEnabled(state == Qt::Checked);
}

void iALogWidget::toggleVTK(int state)
{
	m_redirectVTK->setEnabled(state == Qt::Checked);
}

bool iALogWidget::logVTK() const
{
	assert(cbVTK->isChecked() == m_redirectVTK->enabled());
	return m_redirectVTK->enabled();
}

bool iALogWidget::logITK() const
{
	assert(cbITK->isChecked() == m_redirectITK->enabled());
	return m_redirectITK->enabled();
}

void iALogWidget::setLogVTK(bool enabled)
{
	cbVTK->setChecked(enabled);
}

void iALogWidget::setLogITK(bool enabled)
{
	cbITK->setChecked(enabled);
}
