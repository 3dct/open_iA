// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QString>

//! Interface for anything that can be saved as a project.
//! Necessary since not all current tools employ iAMdiChild as container for their widgets.
//! So every such container needs to implement this class and its doLoadProject method,
//! in order to be called when the user selects to "Save Project".
//!
//! Refactoring ideas:
//! - Move dataset rendering capabilities from iAMdiChild / MdiChild into a separate "viewer"
//!   tool, and make all separate tools using this as base class (only iAFeatureAnalyzer!) a
//!   tool under iAMdiChild
class iAguibase_API iASavableProject
{
public:
	//! Called from main window to save the project of the current window.
	//! In case you're wondering why there are two methods in this class, this one
	//! and the virtual "doSaveProject": This is because it follows the "Non-Virtual Interface
	//! Idiom", see http://www.gotw.ca/publications/mill18.htm
	bool saveProject(QString const & basePath);
	//! return the name of the last file that was stored
	QString const & fileName() const;
protected:
	//! Prevent destruction of the object through this interface.
	virtual ~iASavableProject();
private:
	//! Override this method to implement the actual saving of the project
	virtual bool doSaveProject(QString const & projectFileName) = 0;

	QString m_fileName;
};
