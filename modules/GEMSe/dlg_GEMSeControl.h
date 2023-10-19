// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_GEMSeControl.h"

#include "iAImageTreeNode.h"    // for LabelImagePointer

#include <iAProgress.h>

#include <qthelper/iAQTtoUIConnector.h>

#include <QMap>

class iAImageClusterer;
class iAImageSampler;

class dlg_GEMSe;
class dlg_Consensus;
class dlg_samplings;
class iASamplingSettingsDlg;
class iAColorTheme;
class iASimpleLabelInfo;
class iASamplingResults;

class vtkImageData;

class QSettings;

typedef iAQTtoUIConnector<QDockWidget, Ui_GEMSeControl>   dlg_GEMSeControlUI;

class dlg_GEMSeControl: public dlg_GEMSeControlUI
{
	Q_OBJECT
public:
	dlg_GEMSeControl(
		QWidget *parentWidget,
		dlg_GEMSe* dlgGEMSe,
		//std::vector<std::shared_ptr<iADataSet>> const & dataSets,
		dlg_samplings* dlgSamplings,
		iAColorTheme const * colorTheme
	);
	bool loadSampling(QString const & fileName, int labelCount, int datasetID);
	bool loadClustering(QString const & fileName);
	bool loadRefImg(QString const & refImgName);
	//! Save the GEMSe project to the given settings file.
	void saveProject(QSettings & metaFile, QString const & fileName);
	void exportAttributeRangeRanking();
	void exportRankings();
	void importRankings();
	void setSerializedHiddenCharts(QString const & hiddenCharts);
	void setLabelInfo(QString const & colorTheme, QString const & labelNames);
	void exportIDs();
private slots:
	void startSampling();
	void samplingFinished();
	void clusteringFinished();
	void loadSamplingSlot();
	void loadClusteringSlot();
	void calculateClustering();
	void saveAll();
	void saveClustering();
	void saveDerivedOutputSlot();
	void dataAvailable();
	void dataSetSelected(size_t dataSetIdx);
	void SetIconSize(int newSize);
	void setColorTheme(int index);
	void setRepresentative(int index);
	void loadRefImgSlot();
	void setMagicLensCount(int);
	void freeMemory();
	void setProbabilityProbing(int);
	void setCorrectnessUncertainty(int);
	void dataTFChanged();
private:
	void saveGEMSeProject(QString const & fileName, QString const & hiddenCharts);
	void EnableClusteringDependantUI();
	void EnableSamplingDependantUI();
	void saveDerivedOutput(
		QString const & derivedOutputFileName,
		QString const & attributeDescriptorOutputFileName,
		std::shared_ptr<iASamplingResults> results);

	iASamplingSettingsDlg* m_dlgSamplingSettings;
	dlg_GEMSe* m_dlgGEMSe;
	dlg_samplings* m_dlgSamplings;
	dlg_Consensus* m_dlgConsensus;

	iAProgress m_progress;
	std::shared_ptr<iAImageSampler> m_sampler;
	std::shared_ptr<iAImageClusterer> m_clusterer;

	QString m_outputFolder, m_cltFile, m_m_metaFileName;
	std::shared_ptr<iASimpleLabelInfo> m_simpleLabelInfo;
	LabelImagePointer m_refImg;
	QVariantMap m_samplingSettings;
};
