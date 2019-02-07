/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iABatchProcessModuleInterface.h"

#include "iABatchFilter.h"
#include "iAStackReaderFilter.h"

#include <iAFilterRegistry.h>

void iABatchProcessModuleInterface::Initialize()
{
	REGISTER_FILTER(iABatchFilter);
	REGISTER_FILTER(iAStackReaderFilter);
	if (!m_mainWnd)
		return;
	/*
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuEnsembles = getMenuWithTitle( toolsMenu, QString( "Image Ensembles" ), false );
	QAction * actionSample = new QAction( m_mainWnd );
	actionSample->setText(QApplication::translate("MainWindow", "Sample...", 0));
	AddActionToMenuAlphabeticallySorted(menuEnsembles, actionSample, true);
	connect(actionSample, SIGNAL(triggered()), this, SLOT(ShowSampleDialog()));
	*/
}

/*
void iABatchProcessModuleInterface::ShowSampleDialog()
{
	PrepareActiveChild();
	if (!m_mdiChild)
		return;
}
*/
