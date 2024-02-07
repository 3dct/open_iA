// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompVisModuleInterface.h"

//testing
#include "iALog.h"

#include "dlg_CSVReader.h"
#include "iACompVisMain.h"

#include <iAMainWindow.h>

#include <QAction>
#include <QMessageBox>


void iACompVisModuleInterface::Initialize()
{
	
	if (!m_mainWnd)  // if m_mainWnd is not set, we are running in command line mode
	{
		return;  // in that case, we do not do anything as we can not add a menu entry there
	}
	
	QMenu* toolsMenu = m_mainWnd->toolsMenu();
	QAction* actionCompVis = new QAction(QObject::tr("CompVis"), nullptr);
	addToMenuSorted(toolsMenu, actionCompVis);
	connect(actionCompVis, SIGNAL(triggered()), this, SLOT(CompVis()));
}

void iACompVisModuleInterface::CompVis()
{
	iACompVisMain::start(m_mainWnd);
}
