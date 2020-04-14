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
#include "iAModuleInterface.h"

#include "iAModuleDispatcher.h"
#include "iAModuleAttachmentToChild.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <itkMacro.h>    // for itk::ExceptionObject

#include <QMessageBox>

void iAModuleInterface::PrepareResultChild( QString const & title )
{
	m_mdiChild = m_mainWnd->resultChild( title + " " + m_mainWnd->activeMdiChild()->windowTitle().replace("[*]",""));
	if( !m_mdiChild )
	{
		m_mainWnd->statusBar()->showMessage( "Cannot get result child from main window!", 5000 );
		return;
	}
}

void iAModuleInterface::PrepareResultChild( int childInd, QString const & title )
{
	m_mdiChild = m_mainWnd->resultChild( childInd, title );
	if( !m_mdiChild )
	{
		m_mainWnd->statusBar()->showMessage( "Cannot get result child from main window!", 5000 );
		return;
	}
}

void iAModuleInterface::SetMainWindow( MainWindow * mainWnd )
{
	m_mainWnd = mainWnd;
}

void iAModuleInterface::SetDispatcher( iAModuleDispatcher * dispatcher )
{
	m_dispatcher = dispatcher;
}

iAModuleInterface::iAModuleInterface():
	m_mainWnd(nullptr),
	m_dispatcher(nullptr),
	m_mdiChild(nullptr)
{}

void iAModuleInterface::PrepareActiveChild()
{
	m_mdiChild = m_mainWnd->activeMdiChild();
	if( !m_mdiChild )
	{
		m_mainWnd->statusBar()->showMessage( "Cannot get active child from main window!", 5000 );
		return;
	}
}

QMenu * iAModuleInterface::getMenuWithTitle( QMenu * parentMenu, QString const & title, bool isDisablable /*= true*/  )
{
	return m_dispatcher->getMenuWithTitle(parentMenu, title, isDisablable);
}

void iAModuleInterface::SaveSettings() const {}

void iAModuleInterface::ChildCreated(MdiChild * /*child*/)
{
}

iAModuleInterface::~iAModuleInterface()
{
	for( int i = 0; i < m_attachments.size(); ++i )
	{
		if( m_attachments[i] )
			delete m_attachments[i];	// should probably be deleted in module dll as its created there?
	}
}

void iAModuleInterface::detachChild(MdiChild* child)
{
	if( !child )
		return;
	for( int i = 0; i < m_attachments.size(); ++i )
	{
		if( m_attachments[i]->getMdiChild() == child )
		{
			delete m_attachments[i];	// should probably be deleted in module dll as its created there?
			m_attachments.remove( i );
		}
	}
}

void iAModuleInterface::attachedChildClosed()
{
	MdiChild * sender = dynamic_cast<MdiChild*> (QObject::sender());
	detachChild(sender);
}

void iAModuleInterface::detach()
{
	iAModuleAttachmentToChild * attachment= dynamic_cast<iAModuleAttachmentToChild*> (QObject::sender());
	detachChild(attachment->getMdiChild());
}

bool iAModuleInterface::isAttached()
{
	//check if already attached
	for( int i = 0; i < m_attachments.size(); ++i )
	{
		if( m_attachments[i]->getMdiChild() == m_mdiChild )
			return true;
	}
	return false;
}

void iAModuleInterface::AddActionToMenuAlphabeticallySorted( QMenu * menu, QAction * action, bool isDisablable /*= true */ )
{
	m_dispatcher->AddActionToMenuAlphabeticallySorted(menu, action, isDisablable);
}

iAModuleAttachmentToChild * iAModuleInterface::CreateAttachment( MainWindow * /*mainWnd*/, MdiChild * /*child*/ )
{
	return nullptr;
}

bool iAModuleInterface::AttachToMdiChild( MdiChild * child )
{
	//check if already attached
	m_mdiChild = child;
	if( isAttached() )
		return false;
	//create attachment
	try
	{
		iAModuleAttachmentToChild * attachment = CreateAttachment( m_mainWnd, child );
		if( !attachment )
			return false;
		//add an attachment
		m_attachments.push_back( attachment );
		connect( child, SIGNAL( closed() ), this, SLOT( attachedChildClosed() ) );
		connect ( attachment, SIGNAL( detach() ), this, SLOT (detach() ) );
	}
	catch( itk::ExceptionObject &excep )
	{  // check why we catch an ITK exception here! in the attachment initialization, no ITK filters should be called...
		child->addMsg( tr("%1 in File %2, Line %3").arg( excep.GetDescription() )
			.arg( excep.GetFile() )
			.arg( excep.GetLine() ) );
		return false;
	}
	return true;
}
