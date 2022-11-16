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
#include "iALabellingModuleInterface.h"

#include "iAAnnotationTool.h"
#include "iALabellingTool.h"

#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h>
#include <iAToolRegistry.h>

#include <QAction>
#include <QMenu>

void iALabellingModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	auto actionLabelling = new QAction(tr("Labelling"), m_mainWnd);
	connect(actionLabelling, &QAction::triggered, this, [this]()
		{
			addToolToActiveMdiChild<iALabellingTool>(iALabellingTool::Name, m_mainWnd);
		});

	auto actionAnnotation = new QAction(tr("Annotations"), m_mainWnd);
	connect(actionAnnotation, &QAction::triggered, this, [this]()
		{
			addToolToActiveMdiChild<iAAnnotationTool>(iAAnnotationTool::Name, m_mainWnd);
		});

	auto menuEnsembles = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Labelling"), false);
	menuEnsembles->addAction(actionLabelling);
	menuEnsembles->addAction(actionAnnotation);

	//iAToolRegistry::addTool(iAAnnotationTool::Name, iAAnnotationTool::create)
}
