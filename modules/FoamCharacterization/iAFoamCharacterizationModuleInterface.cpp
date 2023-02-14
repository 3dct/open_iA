// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationModuleInterface.h"

#include "iAFoamCharacterizationTool.h"

#include <iAToolHelper.h>    // for addToolToActiveMdiChild

#include <QAction>

void iAFoamCharacterizationModuleInterface::Initialize( )
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction* actionFoamCharacterization(new QAction(tr("Foam characterization"), m_mainWnd));
	connect(actionFoamCharacterization, &QAction::triggered, this, &iAFoamCharacterizationModuleInterface::slotFoamCharacterization);
	m_mainWnd->makeActionChildDependent(actionFoamCharacterization);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionFoamCharacterization);
}

void iAFoamCharacterizationModuleInterface::slotFoamCharacterization()
{
	addToolToActiveMdiChild<iAFoamCharacterizationTool>("FoamCharacterization", m_mainWnd);
}
