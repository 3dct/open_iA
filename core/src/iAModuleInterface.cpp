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
 
#include "pch.h"
#include "iAModuleInterface.h"

#include "iAModuleDispatcher.h"
#include "iAModuleAttachmentToChild.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <itkMacro.h>  // for itk::ExceptionObject

#include <QMessageBox>
#include <QMdiSubWindow>

void iAModuleInterface::PrepareResultChild( QString const & wndTitle )
{
	m_mdiChild = m_mainWnd->GetResultChild( wndTitle + " " + m_mainWnd->activeMdiChild()->windowTitle());
	if( !m_mdiChild )
	{
		m_mainWnd->statusBar()->showMessage( "Cannot get result child from main window!", 5000 );
		return;
	}
	UpdateChildData();
}

void iAModuleInterface::PrepareResultChild( int childInd, QString const & title )
{
	m_mdiChild = m_mainWnd->GetResultChild( childInd, title );
	if( !m_mdiChild )
	{
		m_mainWnd->statusBar()->showMessage( "Cannot get result child from main window!", 5000 );
		return;
	}
	UpdateChildData();
}

void iAModuleInterface::SetMainWindow( MainWindow * mainWnd )
{
	m_mainWnd = mainWnd;
}

void iAModuleInterface::SetDispatcher( iAModuleDispatcher * dispatcher )
{
	m_dispatcher = dispatcher;
}

iAModuleInterface::iAModuleInterface() {}

void iAModuleInterface::PrepareActiveChild()
{
	m_mdiChild = m_mainWnd->activeMdiChild();
	if( !m_mdiChild )
	{
		m_mainWnd->statusBar()->showMessage( "Cannot get active child from main window!", 5000 );
		return;
	}
	UpdateChildData();
}

MdiChild * iAModuleInterface::GetSecondNonActiveChild() const
{
	QList<QMdiSubWindow *> mdiwindows = m_mainWnd->MdiChildList();
	if( mdiwindows.size() > 2 )
	{
		QMessageBox::warning( m_mainWnd, tr( "Warning" ),
			tr( "Only two datasets can be processed at a time! Please close %1 datasets" )
			.arg( mdiwindows.size() - 2 ) );
		return 0;
	}
	else if( mdiwindows.size() < 2 )
	{
		QMessageBox::warning( m_mainWnd, tr( "Warning" ),
			tr( "Only one dataset available. Please load second dataset for fusion!" ) );
		return 0;
	}
	MdiChild * activeChild = m_mainWnd->activeMdiChild();
	MdiChild * result;
	if( activeChild == qobject_cast<MdiChild *>(mdiwindows.at( 0 )->widget()) )
		result = qobject_cast<MdiChild *>(mdiwindows.at( 1 )->widget());
	else
		result = qobject_cast<MdiChild *>(mdiwindows.at( 0 )->widget());
	return result;
}

QMenu * iAModuleInterface::getMenuWithTitle( QMenu * parentMenu, QString const & title, bool isDisablable /*= true*/  )
{
	QList<QMenu*> submenus = parentMenu->findChildren<QMenu*>();
	for( int i = 0; i < submenus.size(); ++i )
	{
		if( submenus.at( i )->title() == title )
			return  submenus.at( i );
	}
	QMenu * result = new QMenu(parentMenu);
	result->setTitle( title );
	AddActionToMenuAlphabeticallySorted( parentMenu, result->menuAction(), isDisablable );
	return result;
}

void iAModuleInterface::SaveSettings() const {}

void iAModuleInterface::UpdateChildData()
{
	m_childData = iAChildData( m_mdiChild );
}

iAChildData iAModuleInterface::GetChildData() const
{
	return m_childData;
}

void iAModuleInterface::ChildCreated(MdiChild* child)
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
		if( m_attachments[i]->GetMdiChild() == child )
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
	detachChild(attachment->GetMdiChild());
}

bool iAModuleInterface::isAttached()
{
	//check if already attached
	for( int i = 0; i < m_attachments.size(); ++i )
	{
		if( m_attachments[i]->GetMdiChild() == m_mdiChild )
			return true;
	}
	return false;
}

void iAModuleInterface::AddActionToMenuAlphabeticallySorted( QMenu * menu, QAction * action, bool isDisablable /*= true */ )
{
	m_dispatcher->AddModuleAction( action, isDisablable );
	foreach( QAction * curAct, menu->actions() )
	{
		if( curAct->text() > action->text() )
		{
			menu->insertAction( curAct, action );
			return;
		}
	}
	menu->addAction( action );
}

iAModuleAttachmentToChild * iAModuleInterface::CreateAttachment( MainWindow* mainWnd, iAChildData childData )
{
	return 0;
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
		iAModuleAttachmentToChild * attachment = CreateAttachment( m_mainWnd, iAChildData( child ) );
		if( !attachment )
			return false;
		//add an attachment
		m_attachments.push_back( attachment );
		connect( child, SIGNAL( closed() ), this, SLOT( attachedChildClosed() ) );
		connect ( attachment, SIGNAL( detach() ), this, SLOT (detach() ) );
	}
	catch( itk::ExceptionObject &excep )
	{
		child->addMsg( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
			.arg( excep.GetFile() )
			.arg( excep.GetLine() ) );
		return false;
	}
	return true;
}
