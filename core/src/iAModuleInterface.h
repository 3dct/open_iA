/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAModuleAttachmentToChild.h"
#include "open_iA_Core_export.h"

#include <QObject>
#include <QVector>

class iAModuleDispatcher;
class MainWindow;
class MdiChild;

class vtkImageData;
class vtkPolyData;

class QDockWidget;
class QMenu;
class QAction;

//! Base class for a module interface.
//! A class derived from this class, and having a name in the form iA<ModuleName>ModuleInterface needs to be part of each module.
//! E.g. the XRF module needs to contain a class iAXRFModuleInterface.
//! At least the Initialize method needs to be overriden in order to add the custom code of the module to open_iA.
//! This can either be the addition of some filters, or adding an entry directly to open_iA's toolbar or menu.
class open_iA_Core_API iAModuleInterface : public QObject
{
	Q_OBJECT
public:
	iAModuleInterface();
	virtual ~iAModuleInterface();
	//! Called by the module dispatcher on module initialization. There should be no need to call this method from user code
	void SetMainWindow( MainWindow * mainWnd );
	//! Called by the module dispatcher on module initialization. There should be no need to call this method from user code
	void SetDispatcher( iAModuleDispatcher * dispatcher );
	//! Override to add references to the module in the core code, for example menu entries.
	virtual void Initialize() = 0;		// TODO: split up into GUI part and other?
	//! Override to store custom settings of this module; called when program is shut down.
	virtual void SaveSettings() const;
	//! Called whenever an MdiChild object is created. Override to react on this.
	virtual void ChildCreated(MdiChild* child);

protected:
	//! Create a new result child, with a title made from the given title + the previous title of the active child.
	void PrepareResultChild( QString const & title);
	//! Create a new result child at the given index in the MdiChild list with the given title.
	void PrepareResultChild( int childInd, QString const & title );
	//! Set the currently active child as "current".
	void PrepareActiveChild();
	//! Retrieve the menu with the given title (or creates it if it doesn't exist yet).
	QMenu * getMenuWithTitle(QMenu * parentMenu, QString const & title, bool isDisablable = true);
	//! Return true if attached to current mdi child.
	//! @note: current mdi child is determined through m_mdiChild member
	//!       which is _not_ automatically updated to the active mdi child!
	bool isAttached();
	//! Add an action to a given menu ensuring alphabetic order.
	//! @param menu the menu to add the entry to (see e.g. MainWindow::getToolMenu())
	//! @param action the action to add to the menu
	//! @param isDisablable whether the action should be disabled when no child is currently open
	void AddActionToMenuAlphabeticallySorted( QMenu * menu, QAction * action, bool isDisablable = true );
	//! Create a new attachment for the given child.
	virtual iAModuleAttachmentToChild * CreateAttachment( MainWindow* mainWnd, MdiChild * child );
	//! Get an attachment of the current mdi child.
	//! @note current mdi child is determined through m_mdiChild member
	//!       which is _not_ automatically updated to the active mdi child, see m_mdiChild member!
	template <class T> T* GetAttachment();
	//! Sets up a new attachment for the given MdiChild via CreateAttachment and links the two.
	bool AttachToMdiChild( MdiChild * child );

	MainWindow * m_mainWnd;            //!< access to the main window
	iAModuleDispatcher * m_dispatcher; //!< access to the module dispatcher
	//! "current" mdi child
	//! @deprecated use direct access via MainWindow methods
	MdiChild * m_mdiChild;
	//! attachments of this module
	QVector<iAModuleAttachmentToChild*> m_attachments;

private:
	//! called when an MdiChild is closing
	void detachChild(MdiChild* child);

protected slots:
	void attachedChildClosed();
	void detach();
};


template <class T>
T* iAModuleInterface::GetAttachment()
{
	static_assert(std::is_base_of<iAModuleAttachmentToChild, T>::value, "GetAttachment: given type must inherit from iAModuleAttachmentToChild!");
	for (int i = 0; i < m_attachments.size(); ++i)
	{
		if (m_attachments[i]->getMdiChild() == m_mdiChild &&
			dynamic_cast<T*>(m_attachments[i]) != 0)
		{
			return dynamic_cast<T*>(m_attachments[i]);
		}
	}
	return 0;
}
