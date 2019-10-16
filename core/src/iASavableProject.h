/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

//! Interface for anything that can be saved as a project.
//! Necessary since not all current tools employ mdichild as container for their widgets.
//! So every such container needs to implement this class and its doLoadProject method,
//! in order to be called when the user selects to "Save Project".
//!
//! Refactoring ideas:
//! - make this class the container for the open projects currently stored in 
//!   MdiChild::m_projects,
//!   and move MdiChild::doSaveProject into saveProject (or into default implementation of
//!   doSaveProject, as MdiChild probably into the forseeable future needs to do things
//!   differently to be able to load .mod files)
//! - rename to iAOpenProjects or something?
class open_iA_Core_API iASavableProject
{
public:
	//! Called from main window to save the project of the current window.
	//! In case you're wondering why there are two methods in this class, this one
	//! and the virtual "doLoadProject": This is because it follows the "Non-Virtual Interface
	//! Idiom", see http://www.gotw.ca/publications/mill18.htm
	void saveProject();
protected:
	//! Prevent destruction of the object through this interface.
	virtual ~iASavableProject();
private:
	//! Override this method to implement the actual saving of the project
	virtual void doSaveProject() = 0;
};
