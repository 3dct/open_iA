// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iABoneThicknessModuleInterface.h"

#include "iABoneThicknessTool.h"

#include <iAToolHelper.h>    // for addToolToActiveMdiChild

#include <QAction>

void iABoneThicknessModuleInterface::Initialize( )
{
	if (!m_mainWnd)
	{
		return;
	}
	auto actionBoneThickness = new QAction(tr("Bone thickness"), m_mainWnd);
	connect(actionBoneThickness, &QAction::triggered, this, &iABoneThicknessModuleInterface::slotBoneThickness);
	m_mainWnd->makeActionChildDependent(actionBoneThickness);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionBoneThickness);
}

void iABoneThicknessModuleInterface::slotBoneThickness()
{
	addToolToActiveMdiChild<iABoneThicknessTool>("BoneThickness", m_mainWnd);
}
