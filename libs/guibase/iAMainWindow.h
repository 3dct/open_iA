/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAguibase_export.h"

#include <QMainWindow>

class iAMdiChild;
class iAModuleDispatcher;

class QMdiSubWindow;
class QString;

class iAguibase_API iAMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	// Child windows creation / access:

	//! @{ Get access to result child with the given title.
	//! (depending on preferences, this will either open a new mdi child window, or reuse the currently active one)
	virtual iAMdiChild* resultChild(QString const& title) = 0;
	virtual iAMdiChild* resultChild(int childInd, QString const& title) = 0;
	virtual iAMdiChild* resultChild(iAMdiChild* oldChild, QString const& title) = 0;
	//! @}
	//! Create a new child window
	virtual iAMdiChild* createMdiChild(bool unsavedChanges) = 0;
	//! Close a child window
	virtual void closeMdiChild(iAMdiChild* child) = 0;
	//! Close all child windows (with a question whether sure if a child has modified data)
	virtual void closeAllSubWindows() = 0;
	
	//! Provides access to the currently active mdi child, if such is available.
	//! @return pointer to the currently active mdi child, or nullptr if no child is currently open
	virtual iAMdiChild* activeMdiChild() = 0;

	//! Return the QMdiSubWindow for the current child
	virtual QMdiSubWindow* activeChild() = 0;

	//! Get the list of current MdiChild windows.
	virtual QList<iAMdiChild*> mdiChildList() = 0;

	//! add a new widget as sub window in the mdi area, and return the respective mdi subwindow.
	virtual QMdiSubWindow* addSubWindow(QWidget* child) = 0;

	//! Provides access to a second loaded mdi child, if such is available.
	//! Will throw an error if none is available or more than two are loaded.
	//! @deprecated instead of this method, in filters, use the facilities
	//!     provided in iAFilter (via the requiredInputs parameter to the constructor) to specify multiple inputs
	virtual iAMdiChild* secondNonActiveChild() = 0;

	//! load the file under the given filename in a new child window
	virtual void loadFile(QString fileName, bool isStack) = 0;


	// Access to menus:

	//! Get the File menu (can be used by modules to append entries to it).
	virtual QMenu* fileMenu() = 0;
	//! Get the Filters menu (can be used by modules to append entries to it).
	virtual QMenu* filtersMenu() = 0;
	//! Get the Tools menu (can be used by modules to append entries to it).
	virtual QMenu* toolsMenu() = 0;
	//! Get the Help menu (can be used by modules to append entries to it).
	virtual QMenu* helpMenu() = 0;


	// Various:

	//! mark a QAction (typically added to filters or tools menu by a module)
	//! as depending on an MDI child window being open and active
	virtual void makeActionChildDependent(QAction* action) = 0;

	//! retrieve the module dispatcher
	virtual iAModuleDispatcher& moduleDispatcher() const = 0;

	//! whether the current qss theme is bright mode (true) or dark mode (false)
	virtual bool brightMode() const = 0;


	//! Retrieve current directory path (the "working folder")
	virtual QString const& path() const = 0;
	//! Set current directory path (the "working folder")
	virtual void setPath(QString const& p) = 0;
	//! @deprecated. Use a specific mdichilds, or even better, an mdichilds dlg_modalities methods instead!
	virtual QString const& currentFile() const = 0;

signals:
	void styleChanged();
	void histogramAvailable();
};