// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iATool.h"

#include <QString>

#include <memory>

class iAFeatureAnalyzerTool : public iATool
{
public:
	static QString const ID;
	iAFeatureAnalyzerTool(iAMainWindow* mainWnd, iAMdiChild* child);
	void setOptions(QString const& resultsFolder, QString const& datasetsFolder);
	void loadState(QSettings& projectFile, QString const& fileName) override;
	void saveState(QSettings& projectFile, QString const& fileName) override;
private:
	static QString const ResultsFolderKey;
	static QString const DatasetFolderKey;

	QString m_resultsFolder, m_datasetsFolder;
};
