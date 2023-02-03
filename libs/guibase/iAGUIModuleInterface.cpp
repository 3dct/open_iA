// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGUIModuleInterface.h"

#include "iALog.h"
#include "iAMainWindow.h"
#include "iAMdiChild.h"

#include <QStatusBar>
#include <QString>

iAGUIModuleInterface::~iAGUIModuleInterface()
{}

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
