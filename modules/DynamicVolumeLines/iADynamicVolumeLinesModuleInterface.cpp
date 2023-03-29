// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADynamicVolumeLinesModuleInterface.h"

#include "dlg_DynamicVolumeLines.h"

#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <QAction>
#include <QFileDialog>
#include <QMessageBox>

void iADynamicVolumeLinesModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction * actionDynamicVolumeLines = new QAction(tr("Dynamic Volume Lines"), m_mainWnd);
	connect(actionDynamicVolumeLines, &QAction::triggered, this, &iADynamicVolumeLinesModuleInterface::DynamicVolumeLines);
	m_mainWnd->makeActionChildDependent(actionDynamicVolumeLines);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionDynamicVolumeLines);
}

void iADynamicVolumeLinesModuleInterface::DynamicVolumeLines()
{
	QDir datasetsDir = m_mainWnd->activeMdiChild()->filePath();
	datasetsDir.setNameFilters(QStringList("*.mhd"));
	dc = new dlg_DynamicVolumeLines(m_mainWnd->activeMdiChild(), datasetsDir);
	m_mainWnd->activeMdiChild()->addDockWidget(Qt::BottomDockWidgetArea, dc);
	dc->raise();
}
