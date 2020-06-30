/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASavableProject.h"

#include <io/iAIOProvider.h>

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

bool iASavableProject::saveProject(QString const & basePath)
{
	QString projectFileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		QCoreApplication::translate("MainWindow", "Select Output File"),
		basePath,
		iAIOProvider::NewProjectFileTypeFilter + iAIOProvider::ProjectFileTypeFilter);
	if (projectFileName.isEmpty())
	{
		return false;
	}
	m_fileName = projectFileName;
	return doSaveProject(projectFileName);
}

QString const& iASavableProject::fileName() const
{
	return m_fileName;
}

iASavableProject::~iASavableProject()
{}
