/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "open_iA_Core_export.h"

#include "iAFilterRegistry.h"

#include <QMap>

class iAFilterRunnerGUI;
//! For internal use in iAFilterRegistry and iAFilterFactory only.
//! There should be no need to use this class directly; use REGISTER_FILTER or
//! REGISTER_FILTER_WITH_RUNNER macros below instead!
using iAIFilterRunnerGUIFactory = iAGenericFactory<iAFilterRunnerGUI>;
template <typename FilterRunnerGUIType> using iAFilterRunnerGUIFactory = iASpecificFactory<FilterRunnerGUIType, iAFilterRunnerGUI>;

class open_iA_Core_API iAFilterRunnerRegistry
{
public:
	//! Retrieve the callback for a given factory (if the given factory does not
	//! have a callback, nullptr is returned).
	static QSharedPointer<iAIFilterRunnerGUIFactory> filterRunner(int filterID);

	static void addFilterFactory(QSharedPointer<iAIFilterFactory> factory, QSharedPointer<iAIFilterRunnerGUIFactory> runner);

private:
	iAFilterRunnerRegistry() =delete;  //!< iAFilterRunnerRegistry is meant to be used statically only, thus prevent creation of objects
	static QMap<int, QSharedPointer<iAIFilterRunnerGUIFactory>> m_runner;
};

//! Macro to register a class derived from iAFilter in the iAFilterRegistry,
//! along with a runner. In comparison to the macro REGISTER_FILTER, you can provide your
//! own runner class here (derived from iAFilterRunnerGUI to extend or modify the
//! behavior of the filter when run from the GUI.
//! See iAFilterRegistry for more details
#define REGISTER_FILTER_WITH_RUNNER(FilterType, FilterRunnerType)                                           \
	iAFilterRunnerRegistry::addFilterFactory(QSharedPointer<iAIFilterFactory>(new iAFilterFactory<FilterType>()), \
		QSharedPointer<iAIFilterRunnerGUIFactory>(new iAFilterRunnerGUIFactory<FilterRunnerType>()));