// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FeatureAnalyzerHelpers.h"    // for ImagePointer

#include <QThread>

class iAFeatureAnalyzerComputationModuleInterface;

class iADatasetInfo : public QThread
{
	Q_OBJECT

public:
	iADatasetInfo(iAFeatureAnalyzerComputationModuleInterface* pmi);
	QStringList getNewGeneratedInfoFiles();

protected:
	void run() override;
	void calculateInfo();

	iAFeatureAnalyzerComputationModuleInterface* m_pmi;

signals:
	void progress( int );

private:
	template<class T> void generateInfo( QString datasetPath, QString datasetName,
		iAITKIO::ImagePointer & image, iAFeatureAnalyzerComputationModuleInterface * pmi,
		int filesInfoNbToCreate, int currentFInfoNb );

	QStringList m_newGeneratedInfoFilesList;
};
