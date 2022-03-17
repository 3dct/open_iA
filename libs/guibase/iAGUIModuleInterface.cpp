/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAGUIModuleInterface.h"

#include "iALog.h"
#include "iAModuleAttachmentToChild.h"
#include "iAMainWindow.h"
#include "iAMdiChild.h"

#include <itkMacro.h>    // for itk::ExceptionObject

#include <QStatusBar>
#include <QString>

void iAGUIModuleInterface::PrepareResultChild( QString const & title )
{
	m_mdiChild = m_mainWnd->resultChild( title + " " + m_mainWnd->activeMdiChild()->windowTitle().replace("[*]",""));
	if( !m_mdiChild )
	{
		m_mainWnd->statusBar()->showMessage( "Cannot get result child from main window!", 5000 );
		return;
	}
}

void iAGUIModuleInterface::PrepareResultChild( int childInd, QString const & title )
{
	m_mdiChild = m_mainWnd->resultChild( childInd, title );
	if( !m_mdiChild )
	{
		m_mainWnd->statusBar()->showMessage( "Cannot get result child from main window!", 5000 );
		return;
	}
}

void iAGUIModuleInterface::SetMainWindow( iAMainWindow * mainWnd )
{
	m_mainWnd = mainWnd;
}

iAGUIModuleInterface::iAGUIModuleInterface():
	m_mainWnd(nullptr),
	m_mdiChild(nullptr)
{}

void iAGUIModuleInterface::PrepareActiveChild()
{
	m_mdiChild = m_mainWnd->activeMdiChild();
	if( !m_mdiChild )
	{
		m_mainWnd->statusBar()->showMessage( "Cannot get active child from main window!", 5000 );
		return;
	}
}

void iAGUIModuleInterface::SaveSettings() const {}

void iAGUIModuleInterface::ChildCreated(iAMdiChild * /*child*/)
{
}

iAGUIModuleInterface::~iAGUIModuleInterface()
{
	for( int i = 0; i < m_attachments.size(); ++i )
	{
		if( m_attachments[i] )
			delete m_attachments[i];	// should probably be deleted in module dll as its created there?
	}
}

void iAGUIModuleInterface::detachChild(iAMdiChild* child)
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

void iAGUIModuleInterface::attachedChildClosed()
{
	iAMdiChild * sender = dynamic_cast<iAMdiChild*> (QObject::sender());
	detachChild(sender);
}

void iAGUIModuleInterface::detach()
{
	iAModuleAttachmentToChild * attachment= dynamic_cast<iAModuleAttachmentToChild*> (QObject::sender());
	detachChild(attachment->getMdiChild());
}

bool iAGUIModuleInterface::isAttached()
{
	//check if already attached
	for( int i = 0; i < m_attachments.size(); ++i )
	{
		if (m_attachments[i]->getMdiChild() == m_mdiChild)
		{
			return true;
		}
	}
	return false;
}

iAModuleAttachmentToChild * iAGUIModuleInterface::CreateAttachment( iAMainWindow * /*mainWnd*/, iAMdiChild * /*child*/ )
{
	return nullptr;
}

bool iAGUIModuleInterface::AttachToMdiChild( iAMdiChild * child )
{
	//check if already attached
	m_mdiChild = child;
	if (isAttached())
	{
		return false;
	}
	//create attachment
	try
	{
		iAModuleAttachmentToChild * attachment = CreateAttachment( m_mainWnd, child );
		if (!attachment)
		{
			return false;
		}
		//add an attachment
		m_attachments.push_back( attachment );
		connect(child, &iAMdiChild::closed, this, &iAGUIModuleInterface::attachedChildClosed);
		connect(attachment, &iAModuleAttachmentToChild::detach, this, &iAGUIModuleInterface::detach);
	}
	catch( itk::ExceptionObject &excep )
	{	// check why we catch an ITK exception here! in the attachment initialization, no ITK filters should be called...
		// assumption: as e.g. used in FuzzyFeatureTracking, this is just to be able to use
		// the file/line information that the itk exception object holds
		LOG(lvlError, tr("%1 in File %2, Line %3").arg( excep.GetDescription() )
			.arg( excep.GetFile() )
			.arg( excep.GetLine() ) );
		return false;
	}
	return true;
}
