// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

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

//! Helper for registering collections of settings with the iASettingsManager
//! adds a settings collection, available via the given class `Obj`'s `defaultAttributes` method
//! to iASettingsManager under the given `Name`.
//! Simplifies running the registration automatically at "program startup" without having to
//! explicitly
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
};

template <const char* Name, class Obj>
bool iASettingsObject<Name, Obj>::m_sDefaultAttr = iASettingsObject<Name, Obj>::registerDefaultAttributes();
