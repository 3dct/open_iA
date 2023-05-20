// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include <QObject>

//! Base class for a module interface.
//! A class derived from this class (or any class derived from it, see e.g. iAGUIModuleInterface),
//! and having a name in the form iA<ModuleName>ModuleInterface needs to be part of each module.
//! E.g. the FeatureScout module needs to contain a class iAFeatureScoutModuleInterface.
//! At least the Initialize method needs to be overriden in order to add the custom code of the module to open_iA.
//! This can either be the addition of some filters, or adding an entry directly to open_iA's toolbar or menu.
class iAbase_API iAModuleInterface : public QObject
{
	Q_OBJECT
public:
	virtual ~iAModuleInterface();
	//! Override to add references to the module in the core code, for example menu entries.
	virtual void Initialize() = 0;
	//! Override to store custom settings of this module; called when program is shut down.
	virtual void SaveSettings() const;
	//! Called whenever an MdiChild object is created. Override to react on this.

};
