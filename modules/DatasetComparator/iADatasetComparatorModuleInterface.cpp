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
#include "pch.h"
#include "iADatasetComparatorModuleInterface.h"

#include "mainwindow.h"
#include "mdichild.h"

#include <QFileDialog>
#include <QMessageBox>

void iADatasetComparatorModuleInterface::Initialize()
{
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QAction * actionDatasetComparator = new QAction(QApplication::translate("MainWindow", "Dataset Comparator", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(toolsMenu, actionDatasetComparator);
	connect(actionDatasetComparator, SIGNAL(triggered()), this, SLOT(DatasetComparator()));
}

void iADatasetComparatorModuleInterface::DatasetComparator()
{
	PrepareActiveChild();
	QDir datasetsDir = m_mdiChild->getFilePath();
	datasetsDir.setNameFilters(QStringList("*.mhd"));
	dc = new dlg_DatasetComparator(m_mdiChild, datasetsDir);
	m_mdiChild->addDockWidget(Qt::BottomDockWidgetArea, dc);
	dc->raise();
}