// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_Consensus.h"

#include "iAImageTreeNode.h"    // for LabelImagePointer

#include <iAProgress.h>

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

struct ChartWidgetData;
class dlg_GEMSe;
class dlg_samplings;
class iAColorTheme;
class iAImageSampler;
class iALookupTable;
class iASamplingResults;
class iAMdiChild;

class vtkChartXY;
class vtkPlot;
class vtkTable;

class QCheckBox;

typedef iAQTtoUIConnector<QDockWidget, Ui_Consensus>   dlg_ConsensusUI;

class dlg_Consensus : public dlg_ConsensusUI
{
	Q_OBJECT
public:
	dlg_Consensus(iAMdiChild* mdiChild, dlg_GEMSe* dlgGEMSe, int labelCount,
			QString const & folder, dlg_samplings* dlgSamplings);
	virtual ~dlg_Consensus();
	void SetGroundTruthImage(LabelImagePointer groundTruthImage);
	void EnableUI();
private slots:
	void MinAbsPlot();
	void MinDiffPlot();
	void RatioPlot();
	void MaxPixelEntropyPlot();
	void Sample();
	void ClusterUncertaintyDice();
	void StoreResult();
	void StoreConfig();
	void LoadConfig();
	void AbsMinPercentSlider(int);
	void MinDiffPercentSlider(int);
	void MinRatioSlider(int);
	void MaxPixelEntropySlider(int);
	void LabelVoters(int);
	void CheckBoxStateChanged(int);
	void samplerFinished();
	void CalcSTAPLE();
	void CalcMajorityVote();
	void CalcProbRuleVote();
	void SampledItemClicked(QTableWidgetItem *);
private:
	void AddResult(vtkSmartPointer<vtkTable> table, QString const & title);
	int GetWeightType();
	void UpdateWeightPlot();
	void Sample(QVector<std::shared_ptr<iASingleResult> > const & selection, int selectedClusterID, int weightType);
	void SelectionUncertaintyDice(
		QVector<std::shared_ptr<iASingleResult> > const & selection,
		QString const & name);
	void StartNextSampler();

	iAMdiChild*  m_mdiChild;
	dlg_GEMSe* m_dlgGEMSe;
	LabelImagePointer m_groundTruthImage;
	int m_labelCount;
	QVector<vtkSmartPointer<vtkTable> > m_results;
	QMap<QCheckBox*, int> m_checkBoxResultIDMap;
	QMap<int, QVector<vtkIdType> > m_plotMap;
	QVector<ChartWidgetData> m_consensusCharts;

	iAColorTheme const * m_colorTheme;
	iAITKIO::ImagePointer m_lastMVResult;
	QString const & m_folder;

	// for hold-out validation:
	QVector<std::shared_ptr<iAImageSampler> > m_queuedSamplers;
	QVector<QVariantMap > m_samplerParameters;
	std::shared_ptr<iAImageSampler> m_currentSampler;
	iAProgress m_progress;
	QVector<std::shared_ptr<iASamplingResults> > m_comparisonSamplingResults;
	QVector<QVector<int> > m_comparisonBestIDs;
	QVector<QVector<int> > m_comparisonMVIDs;
	QVector<std::shared_ptr<iASingleResult> > m_comparisonBestSelection;
	QVector<std::shared_ptr<iASingleResult> > m_comparisonMVSelection;
	int m_comparisonWeightType;
	dlg_samplings * m_dlgSamplings;
	QString m_cachePath;
};
