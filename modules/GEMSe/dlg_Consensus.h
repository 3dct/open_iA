/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#pragma once

#include "ui_Consensus.h"

#include "iAImageTreeNode.h"    // for LabelImagePointer
#include "qthelper/iAQTtoUIConnector.h"

#include <vtkSmartPointer.h>

typedef iAQTtoUIConnector<QDockWidget, Ui_Consensus>   dlg_ConsensusUI;

struct ChartWidgetData;
class dlg_GEMSe;
class dlg_progress;
class dlg_samplings;
class iAColorTheme;
class iAImageSampler;
class iALookupTable;
class iASamplingResults;
class MdiChild;

class vtkChartXY;
class vtkPlot;
class vtkTable;

class QCheckBox;

class dlg_Consensus : public dlg_ConsensusUI
{
	Q_OBJECT
public:
	dlg_Consensus(MdiChild* mdiChild, dlg_GEMSe* dlgGEMSe, int labelCount,
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
	void SamplerFinished();
	void CalcSTAPLE();
	void CalcMajorityVote();
	void CalcProbRuleVote();
	void SampledItemClicked(QTableWidgetItem *);
private:
	void AddResult(vtkSmartPointer<vtkTable> table, QString const & title);
	int GetWeightType();
	void UpdateWeightPlot();
	void Sample(QVector<QSharedPointer<iASingleResult> > const & selection, int selectedClusterID, int weightType);
	void SelectionUncertaintyDice(
		QVector<QSharedPointer<iASingleResult> > const & selection,
		QString const & name);
	void StartNextSampler();

	MdiChild*  m_mdiChild;
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
	QVector<QSharedPointer<iAImageSampler> > m_queuedSamplers;
	QSharedPointer<iAImageSampler> m_currentSampler;
	dlg_progress * m_dlgProgress;
	QVector<QSharedPointer<iASamplingResults> > m_comparisonSamplingResults;
	QVector<QVector<int> > m_comparisonBestIDs;
	QVector<QVector<int> > m_comparisonMVIDs;
	QVector<QSharedPointer<iASingleResult> > m_comparisonBestSelection;
	QVector<QSharedPointer<iASingleResult> > m_comparisonMVSelection;
	int m_comparisonWeightType;
	dlg_samplings * m_dlgSamplings;
	QString m_cachePath;
};
