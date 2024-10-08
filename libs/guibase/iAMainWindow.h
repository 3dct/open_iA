// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QMainWindow>

class iAFileIO;
class iAMainWindow;
class iAMdiChild;
class iAModuleDispatcher;
class iAPreferences;

class QMdiSubWindow;
class QString;

//! Class used for accessing an existing or creating a new child window as needed.
//! The goal is to defer the need to create windows to as late as possible;
//! while making it customizable whether to create a new window each time one is requested (newWindow);
//! or if newWindow is false, then either the the given child or the same new child is returned each time.
class iAguibase_API iAChildSource
{
public:
	static std::shared_ptr<iAChildSource> make(bool newWindow, iAMdiChild* child = nullptr);
	iAChildSource(bool newWindow, iAMdiChild* child);
	iAMdiChild* child(iAMainWindow* mainWin);
private:
	bool m_newWin;
	iAMdiChild* m_child;
};

//! Abstract interface class for the application's main window, provides access to all global graphical user interface elements.
class iAguibase_API iAMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	// Child windows creation / access:
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

	//! Load a file, either into an existing child window or creating a new one
	//! @param fileName the name of the file (project or dataset) to load
	//! @param childSrc source for the child to put the file into; see iAChildSource
	//! @param io the file io to be used when loading the file. If left at default value nullptr,
	//!     the iAFileTypeRegistry will be consulted to create an io fitting for the given filename
	virtual void loadFile(QString const& fileName, std::shared_ptr<iAChildSource> childSrc = nullptr, std::shared_ptr<iAFileIO> io = nullptr) = 0;


	// Access to menus:

	//! Get the File menu (can be used by modules to append entries to it).
	virtual QMenu* fileMenu() = 0;
	//! Get the File menu (can be used by modules to append entries to it).
	virtual QMenu* editMenu() = 0;
	//! Get the Filters menu (can be used by modules to append entries to it).
	virtual QMenu* filtersMenu() = 0;
	//! Get the Tools menu (can be used by modules to append entries to it).
	virtual QMenu* toolsMenu() = 0;
	//! Get the Help menu (can be used by modules to append entries to it).
	virtual QMenu* helpMenu() = 0;


	// Various:
	//! retrieve the default preferences.
	virtual iAPreferences const& defaultPreferences() const = 0;

	//! retrieve default renderer settings.
	//virtual iARenderSettings const& defaultRenderSettings() const = 0;

	//! mark a QAction (typically added to filters or tools menu by a module)
	//! as depending on an MDI child window being open and active
	virtual void makeActionChildDependent(QAction* action) = 0;

	//! retrieve the module dispatcher
	virtual iAModuleDispatcher& moduleDispatcher() const = 0;

	//! Retrieve current directory path (the "working folder")
	virtual QString const& path() const = 0;
	//! Set current directory path (the "working folder")
	virtual void setPath(QString const& p) = 0;

	//! Access to the main window (more or less singleton);
	static iAMainWindow* get();

	//! add an icon to an action (and keep the action for that icon up-to-date if the style changes
	virtual void addActionIcon(QAction* action, QString const& iconName) =0;

signals:
	//! Triggered whenever the user has changed the style of the program (bright/dark/...) via the preferences.
	void styleChanged();
	//! Triggered whenever the active child window has changed.
	//! Use for example to adapt UI's depending on the current child (toolbars etc.)
	void childChanged();
	//! Triggered whenever a new iAMdiChild instance is created.
	void childCreated(iAMdiChild*);

protected:
	static iAMainWindow* m_mainWnd; //!< the one main window
};
