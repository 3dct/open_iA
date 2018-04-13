/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "iAAlgorithm.h"

#include <vtkSmartPointer.h>

#include <QMap>
#include <QSharedPointer>
#include <QVariant>
#include <QVector>

class iAFilter;
class MainWindow;
class MdiChild;

class vtkImageData;

//! GUI Runner Thread for descendants of iAFilter
//!
//! Used in RunFilter (see below) as thread to run a descendant of iAFilter inside its
//! own thread
class open_iA_Core_API iAFilterRunnerGUIThread : public iAAlgorithm
{
	Q_OBJECT
public:
	iAFilterRunnerGUIThread(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> paramValues, MdiChild* mdiChild);
	void performWork();
	QSharedPointer<iAFilter> Filter();
private:
	QSharedPointer<iAFilter> m_filter;
	QMap<QString, QVariant> m_paramValues;
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
class open_iA_Core_API iAFilterRunnerGUI: public QObject
{
	Q_OBJECT
public:
	//! Method to create an instance of this runner. If you override this class,
	//! don't forget to create your own Create method (called from the factory
	//! template), otherwise still an iAFilterRunnerGUI will be created!
	static QSharedPointer<iAFilterRunnerGUI> Create();

	//! do any potentially necessary GUI preparations (directly before the filter is run)
	virtual void FilterGUIPreparations(QSharedPointer<iAFilter> filter, MdiChild* mdiChild, MainWindow* mainWnd);

	//! Main run method. Calls all the other (non-static) methods in this class.
	//! Override only if you want to change the whole way the filter running works;
	//! typically you will only want to override one of the methods below
	//! @param filter the filter to run
	//! @param mainWnd access to the main window
	virtual void Run(QSharedPointer<iAFilter> filter, MainWindow* mainWnd);

	//! Prompts the user to adapt the parameters to his needs for the current filter run.
	//! @param filter the filter that should be run
	//! @param paramValues the parameter values as loaded from the platform-specific settings store
	//! @param sourceMdi the mdi child that is the main image source for this filter
	//! @param mainWnd access to the main window (as parent for GUI windows)
	virtual bool AskForParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & paramValues,
		MdiChild* sourceMdi, MainWindow* mainWnd, bool askForAdditionalInput);

	//! Loads parameters from the platform-specific store.
	//! @param filter the filter for which to load the parameters
	//! @return a map containing for each parameter name the stored value
	virtual QMap<QString, QVariant> LoadParameters(QSharedPointer<iAFilter> filter, MdiChild* sourceMdi);

	//! Store parameters in the platform-specific store.
	//! @param filter the filter for which to store the parameters
	//! @return a map containing for each parameter name the stored value, as set
	//!     by the user
	virtual void StoreParameters(QSharedPointer<iAFilter> filter, QMap<QString, QVariant> & paramValues);
	
	//! Connect the filter thread to the appropriate signals. If you override this,
	//! you probably will want to still make sure to call this method to make sure
	//! the result gets updated in the mdi child
	//! @param mdiChild the child window into which the results should go
	//! @param thread the thread used to run the filter
	virtual void ConnectThreadSignals(MdiChild* mdiChild, iAFilterRunnerGUIThread* thread);
private slots:
	void FilterFinished();
signals:
	//! Signal which by default (in the default ConnectThreadSignals) is connected
	//! to be emitted at the end of the filter thread run
	void finished();
private:
	QVector<vtkSmartPointer<vtkImageData> > m_additionalInput;
};

#define IAFILTER_RUNNER_CREATE(FilterRunnerName) \
QSharedPointer<iAFilterRunnerGUI> FilterRunnerName::Create() \
{ \
	return QSharedPointer<FilterRunnerName>(new FilterRunnerName()); \
}
