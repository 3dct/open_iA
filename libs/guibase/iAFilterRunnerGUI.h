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

#include "iAguibase_export.h"

#include "iAAbortListener.h"

#include <vtkSmartPointer.h>

#include <QMap>
#include <QThread>
#include <QVariant>
#include <QVector>

#include <memory>

class iADataSet;
class iAFilter;
class iAMainWindow;
class iAMdiChild;

class vtkImageData;

//! GUI Runner Thread for running descendants of iAFilter.
//!
//! Used in iAFilterRunnerGUI::run (see below) as thread to run a descendant of iAFilter inside its
//! own thread
//! needs to be in the .h file so that moc'ing it works.
class iAguibase_API iAFilterRunnerGUIThread : public QThread, public iAAbortListener
{
	Q_OBJECT
public:
	iAFilterRunnerGUIThread(std::shared_ptr<iAFilter> filter, QVariantMap paramValues, iAMdiChild* sourceMDI);
	void run() override;
	std::shared_ptr<iAFilter> filter();
	void addInput(std::shared_ptr<iADataSet> dataSet);
	size_t inputCount() const;
	void abort() override;
	iAMdiChild* sourceMDI();

private:
	std::shared_ptr<iAFilter> m_filter;
	QVariantMap m_paramValues;
	iAMdiChild* m_sourceMDI;
	bool m_aborted;
};


//! Default GUI runner for an iAFilter.
//! For the given descendant of iAFilter, this method loads its settings from
//! the platform-specific settings store (Registry under Windows, .config
//! folder under Unix, ...).
//! Then it shows a dialog to the user to change these parameters.
//! Afterwards it checks the parameters with the given filter.
//! If they are ok, it stores them back to the settings store.
//! Subsequently it creates a thread for the given filter, assigns the slots
//! required for progress indication, final display and cleanup, and finally
//! it runs the filter with the parameters.
class iAguibase_API iAFilterRunnerGUI: public QObject
{
	Q_OBJECT
public:
	//! Method to create an instance of this runner. If you override this class,
	//! don't forget to create your own Create method (called from the factory
	//! template), otherwise still an iAFilterRunnerGUI will be created!
	static std::shared_ptr<iAFilterRunnerGUI> create();

	//! do any potentially necessary GUI preparations (directly before the filter is run)
	virtual void filterGUIPreparations(std::shared_ptr<iAFilter> filter,
		iAMdiChild* mdiChild, iAMainWindow* mainWnd, QVariantMap const& params);

	//! Main run method. Calls all the other (non-static) methods in this class.
	//! Override only if you want to change the whole way the filter running works;
	//! typically you will only want to override one of the methods below
	//! @param filter the filter to run
	//! @param mainWnd access to the main window
	virtual void run(std::shared_ptr<iAFilter> filter, iAMainWindow* mainWnd);

	//! Prompts the user to adapt the parameters to his needs for the current filter run.
	//! @param filter the filter that should be run
	//! @param paramValues the parameter values as loaded from the platform-specific settings store
	//! @param sourceMdi the mdi child that is the main image source for this filter
	//! @param mainWnd access to the main window (as parent for GUI windows)
	//! @param askForAdditionalInput whether the parameter dialog should also ask for additional
	//!     inputs if the filter requires more than 1
	virtual bool askForParameters(std::shared_ptr<iAFilter> filter, QVariantMap & paramValues,
		iAMdiChild* sourceMdi, iAMainWindow* mainWnd, bool askForAdditionalInput);

	//! Loads parameters from the platform-specific store.
	//! @param filter the filter for which to load the parameters
	//! @param sourceMdi the mdi child which was active when the filter was started.
	//!     Not used in the standard implementation, but may be used by overriding classes to access
	//!     properties of the input file, e.g. in the extract image filter it is used to get the size
	//!     of the input image.
	//! @return a map containing for each parameter name the stored value
	virtual QVariantMap loadParameters(std::shared_ptr<iAFilter> filter, iAMdiChild* sourceMdi);

	//! Store parameters in the platform-specific store.
	//! @param filter the filter for which to store the parameters
	//! @param paramValues the parameters and their values
	//! @return a map containing for each parameter name the stored value, as set
	//!     by the user
	virtual void storeParameters(std::shared_ptr<iAFilter> filter, QVariantMap & paramValues);

private slots:
	void filterFinished();
signals:
	//! Signal which by default (in the default ConnectThreadSignals) is connected
	//! to be emitted at the end of the filter thread run
	void finished();
private:
	std::vector<std::shared_ptr<iADataSet>> m_additionalInput;
	QString m_sourceFileName;
};

#define IAFILTER_RUNNER_CREATE(FilterRunnerName) \
std::shared_ptr<iAFilterRunnerGUI> FilterRunnerName::create() \
{ \
	return std::make_shared<FilterRunnerName>(); \
}
