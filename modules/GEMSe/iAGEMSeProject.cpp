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
#include "iAGEMSeProject.h"

#include "iAGEMSeModuleInterface.h"

#include <iAModuleDispatcher.h> // TODO: Refactor; it shouldn't be required to go via iAModuleDispatcher to retrieve one's own module
#include <mainwindow.h>

const QString iAGEMSeProject::ID("FeatureScout");

iAGEMSeProject::iAGEMSeProject()
{}

iAGEMSeProject::~iAGEMSeProject()
{}

void iAGEMSeProject::loadProject(QSettings & projectFile, QString const & fileName)
{
	iAGEMSeModuleInterface * gemseModule = m_mainWindow->getModuleDispatcher().GetModule<iAGEMSeModuleInterface>();
	gemseModule->loadProject(m_mdiChild, projectFile, fileName);
}

void iAGEMSeProject::saveProject(QSettings & projectFile, QString const & fileName)
{
	iAGEMSeModuleInterface * gemseModule = m_mainWindow->getModuleDispatcher().GetModule<iAGEMSeModuleInterface>();
	gemseModule->saveProject(projectFile, fileName);
}

QSharedPointer<iAProjectBase> iAGEMSeProject::create()
{
	return QSharedPointer<iAGEMSeProject>::create();
}
