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
#include "iAMappIntensitiesThread.h"

#include <QFileDialog>

void iADatasetComparatorModuleInterface::Initialize()
{
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuDatasetComparator = getMenuWithTitle(toolsMenu, QString("DatasetComparator"));
	QAction * actionDatasetComparator = new QAction(QApplication::translate("MainWindow", "DatasetComparator", 0), m_mainWnd);
	menuDatasetComparator->addAction(actionDatasetComparator);
	connect(actionDatasetComparator, SIGNAL(triggered()), this, SLOT(computeHilbertPath()));
}

void iADatasetComparatorModuleInterface::DatasetComparator()
{
	PrepareActiveChild();

	m_datasetPath = QFileDialog::getExistingDirectory(m_mdiChild, tr("Datasets Directory"),
		"",
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks);

	if (!m_datasetPath.isEmpty())
	{
		QDir datasetsDir(m_datasetPath);
		datasetsDir.setNameFilters(QStringList("*.mhd"));
		m_datasetList = datasetsDir.entryList();

		m_HPath = PathType::New();
		m_HPath->SetHilbertOrder(3);
		m_HPath->Initialize();

		/*typedef PathType::IndexType IndexType;

		for (unsigned int d = 0; d < path->NumberOfSteps(); d++)
		{
			IndexType index = path->Evaluate(d);

		}*/

		iAMappIntensitiesThread * mit = new iAMappIntensitiesThread(this);
		mit->Init(this);
		mit->start();
	}
}