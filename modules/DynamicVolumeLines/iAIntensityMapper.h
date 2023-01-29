// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dlg_DynamicVolumeLines.h"

class iAIntensityMapper : public QObject
{
	Q_OBJECT

public:
	iAIntensityMapper(iAProgress &iMProgress, QDir datasetsDir, PathID pathID, QList<QPair<QString, QList<icData>>> &datasetIntensityMap,
		QList<vtkSmartPointer<vtkImageData>> &m_imgDataList, double &minEnsembleIntensity, double &maxEnsembleIntensity);
	~iAIntensityMapper();

public slots:
	void process();

signals:
	void finished();
	void error(QString err);

private:
	iAProgress &m_iMProgress;
	QDir m_datasetsDir;
	PathID m_pathID;
	QList<QPair<QString, QList<icData>>> &m_DatasetIntensityMap;
	QList<vtkSmartPointer<vtkImageData>> &m_imgDataList;
	double &m_minEnsembleIntensity, &m_maxEnsembleIntensity;
};
