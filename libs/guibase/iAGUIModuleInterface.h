// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iAModuleInterface.h"
#include "iAQMenuHelper.h"    // to make getOrAddSubmenu / addToMenuSorted available as before

class iAMainWindow;
class iAMdiChild;

//! Base class for a module interface.
//! A class derived from this class, and having a name in the form iA<ModuleName>ModuleInterface needs to be part of each module.
//! E.g. the FeatureScout module needs to contain a class iAFeatureScoutModuleInterface.
//! At least the Initialize method needs to be overriden in order to add the custom code of the module to open_iA.
//! This can either be the addition of some filters, or adding an entry directly to open_iA's toolbar or menu.
class iAguibase_API iAGUIModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	iAGUIModuleInterface();
	virtual ~iAGUIModuleInterface();
	//! Called by the module dispatcher on module initialization. There should be no need to call this method from user code
	void SetMainWindow( iAMainWindow * mainWnd );
	//! Override to add references to the module in the core code, for example menu entries.
	void Initialize() override = 0;		// TODO: split up into GUI part and other?
	//! Override to store custom settings of this module; called when program is shut down.
	void SaveSettings() const override;

protected:
	iAMainWindow * m_mainWnd;            //!< access to the main window
};
