// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPolygonPrimitivesModuleInterface.h"

#include "iAGeometricObjectsDialog.h"

#include <iALog.h>
#include <iAMainWindow.h>

#include <QAction>
#include <QMenu>
#include <QMessageBox>

namespace
{
	const QString Title("Add Polygon Object");
}

void iAPolygonPrimitivesModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction* actionAddObject = new QAction(Title, m_mainWnd);
	connect(actionAddObject, &QAction::triggered, this, &iAPolygonPrimitivesModuleInterface::addObject);
	m_mainWnd->makeActionChildDependent(actionAddObject);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionAddObject);
}

void iAPolygonPrimitivesModuleInterface::addObject()
{
	auto child = m_mainWnd->activeMdiChild();
	if (!child)
	{
		QMessageBox::information(m_mainWnd, Title, "Requires an opened dataset.");
		return;
	}
	if (!m_dlg)
	{
		m_dlg = new iAGeometricObjectsDialog();
		m_dlg->setModal(false);
	}
	m_dlg->setMDIChild(child);
	m_dlg->show();
}
