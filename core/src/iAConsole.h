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
#pragma once

#include "open_iA_Core_export.h"

#include "iALogger.h"

#include <vtkSmartPointer.h>
#include <itkSmartPointer.h>

#include <QObject>
#include <QString>

class dlg_console;
class iARedirectVtkOutput;
class iARedirectItkOutput;

//! Singleton providing access to the global logger object.
class open_iA_Core_API iAGlobalLogger
{
public:
	static void setLogger(iALogger* logger);
	static iALogger* get();
private:
	static iALogger* m_globalLogger;
};

#define DEBUG_LOG(t) { if (iAGlobalLogger::get()) iAGlobalLogger::get()->log(t); }

//! Helper class for (debug) logging purposes, available from everywhere as singleton.
//! Instantiates a dlg_console to log debug messages
//! Note: Typically not used directly, but through DEBUG_LOG macro!
//! TODO: check if we can't reuse logging window here!
class open_iA_Core_API iAConsole: public QObject, public iALogger
{
	Q_OBJECT
public:
	static iAConsole* instance();
	static void closeInstance();
	void log(QString const & text) override;
	void setLogToFile(bool value, QString const & fileName, bool verbose=false);
	bool isLogToFileOn() const;
	QString logFileName() const;
	bool isFileLogError() const;
	bool isVisible() const;
	void setVisible(bool visible);
// decouple logging methods from GUI logging (to allow logging from any thread):
signals:
	void logSignal(QString const & text);
	void consoleVisibilityChanged(bool newVisibility);
private slots:
	void logSlot(QString const & text);
	void consoleClosed();
private:
	iAConsole();
	~iAConsole();

	iAConsole(iAConsole const&)			= delete;
	void operator=(iAConsole const&)	= delete;

	void close();

	QString m_logFileName;
	dlg_console* m_console;
	bool m_logToFile;
	bool m_closed;
	bool m_fileLogError;
	vtkSmartPointer<iARedirectVtkOutput> m_vtkOutputWindow;
	itk::SmartPointer<iARedirectItkOutput> m_itkOutputWindow;
};


//! Some default loggers:

//! A logger whose output is written to the global output console (iAConsole).
class open_iA_Core_API iAConsoleLogger : public iALogger
{
public:
	void log(QString const & msg) override;
	static iAConsoleLogger * get();
private:
	iAConsoleLogger();
	iAConsoleLogger(iAConsoleLogger const&);
	void operator=(iAConsoleLogger const&);
};

//! A logger whose output is written to standard output.
class open_iA_Core_API iAStdOutLogger : public iALogger
{
public:
	void log(QString const & msg) override;
	static iAStdOutLogger * get();
private:
	iAStdOutLogger();
	iAStdOutLogger(iAStdOutLogger const&);
	void operator=(iAStdOutLogger const&);
};
