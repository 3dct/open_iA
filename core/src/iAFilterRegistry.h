/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAGenericFactory.h"

#include <QSharedPointer>
#include <QVector>

class iAFilter;
class iAFilterRunnerGUI;

//! For internal use in iAFilterRegistry and iAFilterFactory only.
//! There should be no need to use this class directly; use REGISTER_FILTER or
//! REGISTER_FILTER_WITH_RUNNER macros below instead!
using iAIFilterFactory = iAGenericFactory<iAFilter>;
template <typename iAFilterDerived> using iAFilterFactory = iASpecificFactory<iAFilterDerived, iAFilter>;

//! For internal use in iAFilterRegistry and iAFilterFactory only.
//! There should be no need to use this class directly; use REGISTER_FILTER or
//! REGISTER_FILTER_WITH_RUNNER macros below instead!
using iAIFilterRunnerGUIFactory = iAGenericFactory<iAFilterRunnerGUI>;
template <typename FilterRunnerGUIType> using iAFilterRunnerGUIFactory = iASpecificFactory<FilterRunnerGUIType, iAFilterRunnerGUI>;

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
	static void addFilterFactory(QSharedPointer<iAIFilterFactory> factory);
	//! Adds a given filter factory to the registry, which will be run with the runner for
	//! which the factory is supplied. REGISTER_FILTER_WITH_RUNNER
	//! provides simplified access to this method.
	static void addFilterFactory(QSharedPointer<iAIFilterFactory> factory,
		QSharedPointer<iAIFilterRunnerGUIFactory> runner);
	//! Retrieve a list of all currently registered filter (factories)
	static QVector<QSharedPointer<iAIFilterFactory>> const & filterFactories();
	//! Retrieve the filter with the given name.
	//! If there is no such filter, a "null" shared pointer is returned
	static QSharedPointer<iAFilter> filter(QString const & name);
	//! Retrieve the ID of the filter with the given name
	//! If there is no such filter, -1 is returned
	static int filterID(QString const & name);
	//! Retrieve the callback for a given factory (if the given factory does not
	//! have a callback, nullptr is returned).
	static QSharedPointer<iAIFilterRunnerGUIFactory> filterRunner(int filterID);
private:
	iAFilterRegistry() =delete;	//!< iAFilterRegistry is meant to be used statically only, thus prevent creation of objects
	static QVector<QSharedPointer<iAIFilterFactory> > m_filters;
	static QVector<QSharedPointer<iAIFilterRunnerGUIFactory> > m_runner;
};

//! Macro to register a class derived from iAFilter in the iAFilterRegistry, with
//! a default GUI runner. See iAFilterRegistry for more details
#define REGISTER_FILTER(FilterType) \
iAFilterRegistry::addFilterFactory(QSharedPointer<iAIFilterFactory>(new iAFilterFactory<FilterType>()));

//! Macro to register a class derived from iAFilter in the iAFilterRegistry,
//! along with a runner. In comparison to the macro above, you can provide your
//! own runner class here (derived from iAFilterRunnerGUI to extend or modify the
//! behavior of the filter when run from the GUI.
//! See iAFilterRegistry for more details
#define REGISTER_FILTER_WITH_RUNNER(FilterType, FilterRunnerType) \
iAFilterRegistry::addFilterFactory(QSharedPointer<iAIFilterFactory>(new iAFilterFactory<FilterType>()), \
	QSharedPointer<iAIFilterRunnerGUIFactory>(new iAFilterRunnerGUIFactory<FilterRunnerType>()));
