// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

class iAMainWindow;
class iAMdiChild;

class QSettings;
class QString;

//! Base class for all tools; a tool is some collection of user interface elements that provides additional visualization or computation capabilities.
//! Typical ways how iATool is used/created:
//! <ul>
//!     <li>a new iATool-derived class is created</li>
//!     <li>if state is available, the loadState method is called (@see loadState)</li>
//! </ul>
class iAguibase_API iATool
{
public:
	//! Create the user interface elements of this tool.
	//! Note: Since this constructor is also called before loading a previous state via loadState,
	//!       it should set up everything that is required for the tool, but should NOT perform any
	//!       user interaction (e.g., asking the user for parameters or names of files to load)
	// implementation (empty) in iAToolRegistry.cpp
	iATool(iAMainWindow* mainWnd, iAMdiChild* child);
	//! virtual destructor, to enable proper destruction in derived classes and to avoid warnings
	virtual ~iATool();
	//! Load the state of the tool from the given settings
	virtual void loadState(QSettings & projectFile, QString const & fileName); // TODO: replace QSettings with QVariantMap? or at least make const (can't currently because of beginGroup/endGroup
	//! Save the current state of the tool, so that the current window can be restored from the stored data via the loadState method
	virtual void saveState(QSettings & projectFile, QString const & fileName);
	//TODO NEWIO: maybe introduce the following functionality/flag:
	//! indicate whether this tool should be loaded only once the rendering of datasets has been fully initialized (true) or if it can be loaded once the child window is created (false)
	// virtual bool waitForRendering() const;
protected:
	iAMdiChild* m_child;
	iAMainWindow* m_mainWindow;
};
