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
#include "iACompVisModuleInterface.h"

//testing
#include "iALog.h"

#include "dlg_CSVReader.h"
#include "iACompVisMain.h"

#include <iAMainWindow.h>

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