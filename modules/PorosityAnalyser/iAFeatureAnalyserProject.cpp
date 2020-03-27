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
#include "iAFeatureAnalyserProject.h"

#include "iAPorosityAnalyserModuleInterface.h"

#include <iAModuleDispatcher.h>
#include <mainwindow.h>

#include <QSettings>

void iAFeatureAnalyserProject::setOptions(QString const& resultsFolder, QString const& datasetsFolder)
{
	m_resultsFolder = resultsFolder;
	m_datasetsFolder = datasetsFolder;
}

QSharedPointer<iAProjectBase> iAFeatureAnalyserProject::create()
{
	return QSharedPointer<iAFeatureAnalyserProject>::create();
}

void iAFeatureAnalyserProject::loadProject(QSettings& projectFile, QString const& /*fileName*/)
{
	m_resultsFolder = projectFile.value(ResultsFolderKey).toString();
	m_datasetsFolder = projectFile.value(DatasetFolderKey).toString();
	iAPorosityAnalyserModuleInterface* featureAnalyser = m_mainWindow->getModuleDispatcher().GetModule<iAPorosityAnalyserModuleInterface>();
	featureAnalyser->startFeatureAnalyser(m_resultsFolder, m_datasetsFolder);
}

void iAFeatureAnalyserProject::saveProject(QSettings& projectFile, QString const& /*fileName*/)
{
	projectFile.setValue(ResultsFolderKey, m_resultsFolder);
	projectFile.setValue(DatasetFolderKey, m_datasetsFolder);
}

QString const iAFeatureAnalyserProject::ID("FeatureAnalyser");
QString const iAFeatureAnalyserProject::ResultsFolderKey("ResultsFolder");
QString const iAFeatureAnalyserProject::DatasetFolderKey("DatasetFolder");
