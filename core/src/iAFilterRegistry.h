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

#include <QSharedPointer>
#include <QVector>

class iAFilter;
class iAFilterRunnerGUI;
class iAFilterRunnerGUIThread;

//! Class for internal use in iAFilterRegistry and iAFilterFactory only.
//! There should be no need to use this class directly; use REGISTER_FILTER or
//! REGISTER_FILTER_WITH_RUNNER macros below instead!
class open_iA_Core_API iAAbstractFilterFactory
{
public:
	virtual QSharedPointer<iAFilter> Create() =0;
};

//! Class for internal use in iAFilterRegistry and iAFilterFactory only.
//! There should be no need to use this class directly; use REGISTER_FILTER or
//! REGISTER_FILTER_WITH_RUNNER macros below instead!
class open_iA_Core_API iAAbstractFilterRunnerGUIFactory
{
public:
	virtual QSharedPointer<iAFilterRunnerGUI> Create() =0;
};

//! Registry for image filters.
//! Use REGISTER_FILTER and REGISTER_FILTER_WITH_RUNNER macros add a filter
//! to the list of filters in this class.
//! All filters registered with these macros will be added to the Filter menu
//! (in a submenu according to their Category(), the menu entry will have the
//! filter name; the menu entry will be disabled until a dataset is loaded.
//! When a dataset is loaded, clicking the menu entry will result in
//! execution of the filter with the currently active window as input.
//! The class is meant to be used statically (Follows the singleton pattern),
//! the creation of objects of this class is thus prohibited.
class open_iA_Core_API iAFilterRegistry
{
public:
	//! Adds a given filter factory to the registry, which will be run with the default
	//! GUI runner. REGISTER_FILTER provide simplified access to this method.
	static void AddFilterFactory(QSharedPointer<iAAbstractFilterFactory> factory);
	//! Adds a given filter factory to the registry, which will be run with the runner for
	//! which the factory is supplied. REGISTER_FILTER_WITH_RUNNER
	//! provides simplified access to this method.
	static void AddFilterFactory(QSharedPointer<iAAbstractFilterFactory> factory,
		QSharedPointer<iAAbstractFilterRunnerGUIFactory> runner);
	//! Retrieve a list of all currently registered filter (factories)
	static QVector<QSharedPointer<iAAbstractFilterFactory>> const & FilterFactories();
	//! Retrieve the callback for a given factory (if the given factory does not
	//! have a callback, nullptr is returned).
	static QSharedPointer<iAAbstractFilterRunnerGUIFactory> FilterRunner(int filterID);
private:
	iAFilterRegistry();	//!< iAFilterRegistry is meant to be used as a singleton, thus prevent creation of objects
	static QVector<QSharedPointer<iAAbstractFilterFactory> > m_filters;
	static QVector<QSharedPointer<iAAbstractFilterRunnerGUIFactory> > m_runner;
};

//! Factory for an iAFilter.
//! There should be no need to use this class directly; use REGISTER_FILTER or
//! REGISTER_FILTER_WITH_RUNNER macros below instead!
template <typename FilterType>
class iAFilterFactory: public iAAbstractFilterFactory
{
public:
	QSharedPointer<iAFilter> Create() override
	{
		return FilterType::Create();
	}
};


//! Factory for an iAFilterRunnerGUI.
//! There should be no need to use this class directly; use
//! REGISTER_FILTER_WITH_RUNNER macro below instead!
template <typename FilterRunnerGUIType>
class iAFilterRunnerGUIFactory : public iAAbstractFilterRunnerGUIFactory
{
public:
	QSharedPointer<iAFilterRunnerGUI> Create() override
	{
		return FilterRunnerGUIType::Create();
	}
};

//! Macro to register a class derived from iAFilter in the iAFilterRegistry, with
//! a default GUI runner. See iAFilterRegistry for more details
#define REGISTER_FILTER(FilterType) \
iAFilterRegistry::AddFilterFactory(QSharedPointer<iAAbstractFilterFactory>(new iAFilterFactory<FilterType>()));

//! Macro to register a class derived from iAFilter in the iAFilterRegistry,
//! along with a runner. In comparison to the macro above, you can provide your
//! own runner class here (derived from iAFilterRunnerGUI to extend or modify the
//! behavior of the filter when run from the GUI.
//! See iAFilterRegistry for more details
#define REGISTER_FILTER_WITH_RUNNER(FilterType, FilterRunnerType) \
iAFilterRegistry::AddFilterFactory(QSharedPointer<iAAbstractFilterFactory>(new iAFilterFactory<FilterType>()), \
	QSharedPointer<iAAbstractFilterRunnerGUIFactory>(new iAFilterRunnerGUIFactory<FilterRunnerType>()));
