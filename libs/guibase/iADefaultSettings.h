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
	static bool add(QString const& name, iAAttributes* attributes);
	//! Retrieve a map of all registered default settings
	static iASettingsMap const& getMap();
	//! Initialize default settings (load stored values, and create menu entry for modifying them)
	static void init();
	//! Store default settings
	static void store();
	//! edit the default settings for a given name
	static void editDefaultSettings(QWidget* parent, QString const& fullName);
private:
	static bool m_initialized;
};

//! Helper for registering collections of settings with the iASettingsManager.
//! Adds a settings collection, available via the given class `Obj`'s static `defaultAttributes`
//! method to iASettingsManager under the given `Name`.
//! Simplifies running the registration semi-automatically at program startup. The registering
//! class just has to call the selfRegister() method, which registers the setting with the
//! iASettingsManager, which takes care of loading stored previous values, and of storing the
//! values at application end.
template <const char* Name, class Obj>
class iASettingsObject
{
public:
	static void selfRegister()
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
               s_registered;
#pragma GCC diagnostic pop
	}
private:
	static bool s_registered;
	// initialization needs to be outside class, otherwise:
	// error C2131: expression did not evaluate to a constant
};

template <const char* Name, class Obj>
bool iASettingsObject<Name, Obj>::s_registered = iASettingsManager::add(Name, &Obj::defaultAttributes());
