/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAChildData.h"
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

class open_iA_Core_API iAModuleInterface : public QObject
{
	Q_OBJECT
public:
	iAModuleInterface();
	virtual ~iAModuleInterface();
	void SetMainWindow( MainWindow * mainWnd );
	void SetDispatcher( iAModuleDispatcher * dispatcher );
	//! override to add references to the module in the core code,
	//! for example menu entries
	virtual void Initialize() = 0;		// TODO: split up into GUI part and other?
	virtual void SaveSettings() const;
	iAChildData GetChildData() const;
	//! Called whenever an MdiChild object is created. Override to react on this
	virtual void ChildCreated(MdiChild* child);
protected:
	void PrepareResultChild( QString const & wndTitle);
	void PrepareResultChild( int childInd, QString const & title );
	void PrepareActiveChild();
	void UpdateChildData();
	MdiChild * GetSecondNonActiveChild() const;
	QMenu * getMenuWithTitle(QMenu * parentMenu, QString const & title, bool isDisablable = true);
	//! return true if attached to current mdi child
	//! note: current mdi child is determined through m_mdiChild member
	//!       which is _not_ automatically updated to the active mdi child!
	bool isAttached();
	void AddActionToMenuAlphabeticallySorted( QMenu * menu, QAction * action, bool isDisablable = true );
	//! create a new attachment for the given child
	virtual iAModuleAttachmentToChild * CreateAttachment( MainWindow* mainWnd, iAChildData childData );
	//! get an attachment of the current mdi child
	//! note: current mdi child is determined through m_mdiChild member
	//!       which is _not_ automatically updated to the active mdi child!
	template <class T> T* GetAttachment();
	bool AttachToMdiChild( MdiChild * child );

protected:
	MainWindow * m_mainWnd;
	iAModuleDispatcher * m_dispatcher;
	MdiChild * m_mdiChild;
	iAChildData m_childData;

	QVector<iAModuleAttachmentToChild*> m_attachments;
private:
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
		if (m_attachments[i]->GetMdiChild() == m_mdiChild &&
			dynamic_cast<T*>(m_attachments[i]) != 0)
		{
			return dynamic_cast<T*>(m_attachments[i]);
		}
	}
	return 0;
}
