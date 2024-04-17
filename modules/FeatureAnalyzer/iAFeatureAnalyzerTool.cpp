// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFeatureAnalyzerTool.h"

#include "iAFeatureAnalyzerModuleInterface.h"

#include <iAModuleDispatcher.h>
#include <iAMainWindow.h>

#include <QSettings>

void iAFeatureAnalyzerTool::setOptions(QString const& resultsFolder, QString const& datasetsFolder)
{
	m_resultsFolder = resultsFolder;
	m_datasetsFolder = datasetsFolder;
}

iAFeatureAnalyzerTool::iAFeatureAnalyzerTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child)
{
}

void iAFeatureAnalyzerTool::loadState(QSettings& projectFile, QString const& /*fileName*/)
{
	m_resultsFolder = projectFile.value(ResultsFolderKey).toString();
	m_datasetsFolder = projectFile.value(DatasetFolderKey).toString();
	auto featureAnalyzer = m_mainWindow->moduleDispatcher().module<iAFeatureAnalyzerModuleInterface>();
	featureAnalyzer->startFeatureAnalyzer(m_resultsFolder, m_datasetsFolder);
}

void iAFeatureAnalyzerTool::saveState(QSettings& projectFile, QString const& /*fileName*/)
{
	projectFile.setValue(ResultsFolderKey, m_resultsFolder);
	projectFile.setValue(DatasetFolderKey, m_datasetsFolder);
}

QString const iAFeatureAnalyzerTool::ID("FeatureAnalyzer");
QString const iAFeatureAnalyzerTool::ResultsFolderKey("ResultsFolder");
QString const iAFeatureAnalyzerTool::DatasetFolderKey("DatasetFolder");
