// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

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
