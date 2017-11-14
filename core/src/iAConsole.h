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
#pragma once

#include "open_iA_Core_export.h"

#include "iALogger.h"

#include <vtkSmartPointer.h>
#include <itkSmartPointer.h>

#include <QObject>

#include <string>
#include <sstream>
#include <QString>

class dlg_console;
class iARedirectVtkOutput;
class iARedirectItkOutput;

class open_iA_Core_API iAGlobalLogger
{
public:
	static void SetLogger(iALogger* logger);
	static iALogger* Get();
private:
	static iALogger* m_globalLogger;
};

#define DEBUG_LOG(t) { if (iAGlobalLogger::Get()) iAGlobalLogger::Get()->Log(t); }

//! Debugging helper class. Instantiates a dlg_console to
//! log debug messages
//! TODO: check if we can't reuse logging window here!
class open_iA_Core_API iAConsole: public QObject, public iALogger
{
	Q_OBJECT
public:
	static iAConsole* GetInstance();
	static void Close();
	void Log(QString const & text) override;
	void SetLogToFile(bool value, QString const & fileName, bool verbose=false);
	bool IsLogToFileOn() const;
	QString GetLogFileName() const;
	bool IsFileLogError() const;
// decouple logging methods from GUI logging (to allow logging from any thread):
signals:
	void LogSignal(QString const & text);
private slots:
	void LogSlot(QString const & text);
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


// Some default loggers:

class open_iA_Core_API iAConsoleLogger : public iALogger
{
public:
	void Log(QString const & msg) override;
	static iAConsoleLogger * Get();
private:
	iAConsoleLogger();
	iAConsoleLogger(iAConsoleLogger const&);
	void operator=(iAConsoleLogger const&);
};

class open_iA_Core_API iAStdOutLogger : public iALogger
{
public:
	void Log(QString const & msg) override;
	static iAStdOutLogger * Get();
private:
	iAStdOutLogger();
	iAStdOutLogger(iAStdOutLogger const&);
	void operator=(iAStdOutLogger const&);
};
