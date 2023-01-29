// Copyright 2016-2023, the open_iA contributors
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

std::shared_ptr<iATool> iAFeatureAnalyzerTool::create(iAMainWindow* mainWnd, iAMdiChild* child)
{
	return std::make_shared<iAFeatureAnalyzerTool>(mainWnd, child);
}

iAFeatureAnalyzerTool::iAFeatureAnalyzerTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child)
{
}

void iAFeatureAnalyzerTool::loadState(QSettings& projectFile, QString const& /*fileName*/)
{
	m_resultsFolder = projectFile.value(ResultsFolderKey).toString();
	m_datasetsFolder = projectFile.value(DatasetFolderKey).toString();
	iAFeatureAnalyzerModuleInterface* featureAnalyzer = m_mainWindow->moduleDispatcher().module<iAFeatureAnalyzerModuleInterface>();
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
