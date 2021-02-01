/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAModuleAttachmentToChild.h"
#include "iAcore_export.h"

#include "iAModuleInterface.h"

#include <QObject>
#include <QVector>

class iAMainWindow;
class iAMdiChild;

class QMenu;
class QAction;

//! Base class for a module interface.
//! A class derived from this class, and having a name in the form iA<ModuleName>ModuleInterface needs to be part of each module.
//! E.g. the FeatureScout module needs to contain a class iAFeatureScoutModuleInterface.
//! At least the Initialize method needs to be overriden in order to add the custom code of the module to open_iA.
//! This can either be the addition of some filters, or adding an entry directly to open_iA's toolbar or menu.
class iAcore_API iAGUIModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	iAGUIModuleInterface();
	virtual ~iAGUIModuleInterface();
	//! Called by the module dispatcher on module initialization. There should be no need to call this method from user code
	void SetMainWindow( iAMainWindow * mainWnd );
	//! Override to add references to the module in the core code, for example menu entries.
	virtual void Initialize() = 0;		// TODO: split up into GUI part and other?
	//! Override to store custom settings of this module; called when program is shut down.
	virtual void SaveSettings() const;
	//! Called whenever an iAMdiChild object is created. Override to react on this.
	// TODO: get rid of this, only one module is using it!
	virtual void ChildCreated(iAMdiChild* child);

protected:

	//! Create a new result child, with a title made from the given title + the previous title of the active child.
	//! @deprecated. Use methods from iAMainWindow directly
	void PrepareResultChild( QString const & title);
	//! Create a new result child at the given index in the iAMdiChild list with the given title.
	//! @deprecated. Use methods from iAMainWindow directly
	void PrepareResultChild( int childInd, QString const & title );
	//! Set the currently active child as "current".
	//! @deprecated. Don't use m_mdichild mechanism, use activeMdiChild from iAMainWindow directly.
	void PrepareActiveChild();
	//! Return true if attached to current mdi child.
	//! @note: current mdi child is determined through m_mdiChild member
	//!       which is _not_ automatically updated to the active mdi child!
	bool isAttached();
	//! Create a new attachment for the given child.
	virtual iAModuleAttachmentToChild * CreateAttachment( iAMainWindow* mainWnd, iAMdiChild * child );
	//! Get an attachment of the current mdi child.
	//! @note current mdi child is determined through m_mdiChild member
	//!       which is _not_ automatically updated to the active mdi child, see m_mdiChild member!
	template <class T> T* GetAttachment();
	//! Sets up a new attachment for the given iAMdiChild via CreateAttachment and links the two.
	bool AttachToMdiChild( iAMdiChild * child );

	iAMainWindow * m_mainWnd;            //!< access to the main window
	//! "current" mdi child
	//! @deprecated use direct access via iAMainWindow methods
	iAMdiChild * m_mdiChild;
	//! attachments of this module
	QVector<iAModuleAttachmentToChild*> m_attachments;

private:
	//! called when an iAMdiChild is closing
	void detachChild(iAMdiChild* child);

protected slots:
	void attachedChildClosed();
	void detach();
};

//! In the given menu, search for a menu with the given title; if it doesn't exist, add (alphabetically sorted).
iAcore_API QMenu* getOrAddSubMenu(QMenu* parentMenu, QString const& title, bool addSeparator=false);

//! Add a given action to a menu, such that the (previously sorted) menu stays alphabetically sorted.
iAcore_API void addToMenuSorted(QMenu* menu, QAction* action);

template <class T>
T* iAGUIModuleInterface::GetAttachment()
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
