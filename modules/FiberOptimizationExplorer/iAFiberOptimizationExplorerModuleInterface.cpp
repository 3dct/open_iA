/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAFiberOptimizationExplorerModuleInterface.h"

#include "iAFiberOptimizationExplorer.h"

#include "mainwindow.h"

#include <QAction>
#include <QFileDialog>

void iAFiberOptimizationExplorerModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QAction * actionFibreOptimizationExploration = new QAction( "Fiber Optimization Explorer" );
	AddActionToMenuAlphabeticallySorted( toolsMenu, actionFibreOptimizationExploration, false );
	connect( actionFibreOptimizationExploration, &QAction::triggered, this, &iAFiberOptimizationExplorerModuleInterface::FibreOptimizationExploration );
}

void iAFiberOptimizationExplorerModuleInterface::FibreOptimizationExploration()
{
	QString path = QFileDialog::getExistingDirectory(m_mainWnd, "Choose Folder containing Result csv", m_mainWnd->getPath());
	if (path.isEmpty())
		return;
	
	auto explorer = new iAFiberOptimizationExplorer(path);
	m_mainWnd->addSubWindow(explorer);
	explorer->show();
}
