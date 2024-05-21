// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMainWindow.h"

std::shared_ptr<iAChildSource> iAChildSource::make(bool newWindow, iAMdiChild* child)
{
	return std::make_shared<iAChildSource>(newWindow, child);
}

iAChildSource::iAChildSource(bool newWindow, iAMdiChild* child) :
	m_newWin(newWindow),
	m_child(child)
{}

iAMdiChild* iAChildSource::child(iAMainWindow* mainWin)
{
	if (m_newWin || !m_child)
	{
		m_child = mainWin->createMdiChild(false);
	}
	return m_child;
}

iAMainWindow* iAMainWindow::m_mainWnd = nullptr;

iAMainWindow* iAMainWindow::get()
{
	return m_mainWnd;
}
