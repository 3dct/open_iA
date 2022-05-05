/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "iALogger.h"
#include "ui_log.h"

#include <vtkSmartPointer.h>
#include <itkSmartPointer.h>

#include <QString>

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
	//! @{ prevent copying:
	iALogWidget(iALogWidget const&)    = delete;
	void operator=(iALogWidget const&) = delete;
	//! @}
	void closeEvent(QCloseEvent* event) override;

	QString m_logFileName;
	bool m_logToFile;
	iALogLevel m_fileLogLevel;
	bool m_closed;
	bool m_fileLogError;
	bool m_openOnNewMessage;
	vtkSmartPointer<iALogRedirectVTK> m_redirectVTK;
	itk::SmartPointer<iALogRedirectITK> m_redirectITK;
};
