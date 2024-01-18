// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADreamCasterModuleInterface.h"

#include "iADreamCaster.h"

#include <iAMainWindow.h>

#include <QAction>
#include <QFileDialog>
#include <QStatusBar>

void iADreamCasterModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction * actionDreamcaster_Open_file = new QAction(tr("DreamCaster"), m_mainWnd);
	connect(actionDreamcaster_Open_file, &QAction::triggered, this, &iADreamCasterModuleInterface::dreamcasterOpenFile);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionDreamcaster_Open_file);
}

void iADreamCasterModuleInterface::dreamcasterOpenFile()
{
	QString fileName = QFileDialog::getOpenFileName( m_mainWnd, tr( "Open File" ), m_mainWnd->path(), tr( "STL files (*.stl)" ) );
	if( (QFileInfo( fileName ).suffix() == "stl") || (QFileInfo( fileName ).suffix() == "STL") )
	{
		iADreamCaster *child = new iADreamCaster( m_mainWnd );
		m_mainWnd->addSubWindow( child );
		child->loadFile( fileName );
		child->show();
	}
}
