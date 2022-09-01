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

#include "iAbase_export.h"

#include <memory>
#include <vector>

class iAFilter;

class QString;

using iAFilterCreateFuncPtr = std::shared_ptr<iAFilter>(*)();

//! Registry for image filters.
//! Deriving a filter from iAAutoRegistration automatically registers the filter
//! with this registry.
//! All filters registered with these macros will be added to the Filter menu
//! (in a submenu according to their Category(), the menu entry will have the
//! filter name; the menu entry will be disabled until a dataset is loaded.
//! When a dataset is loaded, clicking the menu entry will result in
//! execution of the filter with the currently active window as input.
//! The class is meant to be used statically (Follows the singleton pattern),
//! the creation of objects of this class is thus prohibited.
class iAbase_API iAFilterRegistry
{
public:
	//! Adds a given filter factory to the registry, which will be run with the default
	//! GUI runner. Filters that derive from iAAutoRegistration for automatic registration
	//! do not need to explicitly call this function
	static bool add(iAFilterCreateFuncPtr filterCreateFunc);
	//! Retrieve a list of all currently registered filter (factories)
	static std::vector<iAFilterCreateFuncPtr> const & filterFactories();
	//! Retrieve the filter with the given name.
	//! If there is no such filter, a "null" shared pointer is returned
	static std::shared_ptr<iAFilter> filter(QString const & name);
	//! Retrieve the ID of the filter with the given name
	//! If there is no such filter, -1 is returned
	static int filterID(QString const & name);
private:
	iAFilterRegistry() =delete;	//!< iAFilterRegistry is meant to be used statically only, thus prevent creation of objects
};

