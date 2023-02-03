// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <defines.h>
#include <iAGUIModuleInterface.h>

class iAFeatureAnalyzer;

class iAFeatureAnalyzerModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT

public:
	void Initialize() override;
	void startFeatureAnalyzer(QString const& resultsFolderName, QString const& datasetsFolderName);

private slots:
	void launchFeatureAnalyzer();

private:
	iAFeatureAnalyzer * m_featureAnalyzer;
};
