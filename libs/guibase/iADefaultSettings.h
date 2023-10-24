// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <iAAttributes.h>

//! Manager for collections of settings.
//! You can add a collection of settings (iAAttributes) under a name.
//! This class will store and load default values for these attributes;
//! additionally, a menu entry will be created for each registered settings collection.
//! Use iASettingsObject for making sure the registration will happen early during program startup.
class iAguibase_API iASettingsManager
{
public:
	using iASettingsMap = QMap<QString, iAAttributes*>;
	//! Register a new list of default settings
	static void add(QString const& name, iAAttributes* attributes);
	//! Retrieve a map of all registered default settings
	static iASettingsMap const& getMap();
	//! Initialize default settings (load stored values, and create menu entry for modifying them)
	static void init();
	//! Store default settings
	static void store();
};

//! Helper for registering collections of settings with the iASettingsManager.
//! Adds a settings collection, available via the given class `Obj`'s static `defaultAttributes`
//! method to iASettingsManager under the given `Name`.
//! Simplifies running the registration automatically at "program startup" without having to
//! explicitly call some registration function within the class; the iASettingsManager
//! takes care of loading stored previous values at application start, and of storing the
//! values at application end.
//! **Note** the class deriving from iASettingsObject needs to be marked as "exported"
//! from the shared library it is contained in, otherwise auto-registration will not work!
template <const char* Name, class Obj>
class iASettingsObject
{
	static bool registerDefaultAttributes()
	{
		iASettingsManager::add(Name, &Obj::defaultAttributes());
		return true;
	}
	static bool m_sDefaultAttr;
	// initialization needs to be outside class, since this is not working:
	// ... m_sDefaultAttr = iASettingsObject<Name, Obj>::registerDefaultAttributes();
	// error C2131: expression did not evaluate to a constant
public:
	// required for clang; without this, no self registration there (optimized away?)
	iASettingsObject()
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
		m_sDefaultAttr;
#pragma GCC diagnostic pop
	}
};

template <const char* Name, class Obj>
bool iASettingsObject<Name, Obj>::m_sDefaultAttr = iASettingsObject<Name, Obj>::registerDefaultAttributes();
