// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iALogger.h"
#include "ui_log.h"

#include <vtkSmartPointer.h>
#include <itkSmartPointer.h>

#include <QString>
#include <QObject>  // for Q_DISABLE_COPY_MOVE

class iALogRedirectVTK;
class iALogRedirectITK;

class QDockWidget;

//! A dock widget to show log messages.
//!
//! Implements singleton pattern, as only one instance should exist per application window.
//! Typically you should not use this directly, but use the LOG macro in iALog.h instead!
class iALogWidget: public QDockWidget, public Ui_Log, public iALogger
{
	Q_OBJECT
public:
	//! singleton pattern - retrieve logger instance
	static iALogWidget* get();
	static void shutdown();
	//! log given text with given log level
	void log(iALogLevel lvl, QString const & text) override;
	//! enable/disable logging to file with given name
	void setLogToFile(bool enable, QString const& fileName, bool verbose = false);
	//! whether logging to file is enabled
	bool isLogToFileOn() const;
	//! the name of the file used for logging
	QString logFileName() const;
	//! whether an error occurred when logging to file
	bool isFileLogError() const;
	//! set log level for the log written to file
	//! (can diverge from the log in the console!)
	void setFileLogLevel(iALogLevel lvl);
	//! retrieve current log level for file
	iALogLevel fileLogLevel() const;
	//! override base class log level setting to make sure that
	//! shown combobox gets updated
	void setLogLevel(iALogLevel lvl) override;
	//! sets whether the log window should open when new messages come in
	void setOpenOnNewMessage(bool openOnNewMessage);
	//! whether logging of VTK messages is enabled
	bool logVTK() const;
	//! enabled/disable logging of VTK messages is enabled
	void setLogVTK(bool enabled);
	//! whether logging of ITK messages is enabled
	bool logITK() const;
	//! enabled/disable logging of ITK messages is enabled
	void setLogITK(bool enabled);
signals:
	//! decouple logging methods from GUI logging (to allow logging from any thread):
	void logSignal(int lvl, QString const & text);
private slots:
	void logSlot(int lvl, QString const & text);
	void setLogLevelSlot(int selectedIdx);
	void clear();
	void toggleITK(int state);
	void toggleVTK(int state);

private:
	//! private constructor - retrieve (single) instance via get!
	iALogWidget();
	//! destructor explicitly implemented to avoid having to include iALogRedirectITK
	~iALogWidget();
	Q_DISABLE_COPY_MOVE(iALogWidget);
	void closeEvent(QCloseEvent* event) override;
	void addText(QString const text);

	QString m_logFileName;
	bool m_logToFile;
	iALogLevel m_fileLogLevel;
	bool m_closed;
	bool m_fileLogError;
	bool m_openOnNewMessage;
	vtkSmartPointer<iALogRedirectVTK> m_redirectVTK;
	itk::SmartPointer<iALogRedirectITK> m_redirectITK;
};
