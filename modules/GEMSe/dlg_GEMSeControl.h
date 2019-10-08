/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "ui_GEMSeControl.h"

#include "iAImageTreeNode.h"    // for LabelImagePointer

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QMap>

class iAImageClusterer;
class iAImageSampler;

class dlg_GEMSe;
class dlg_labels;
class dlg_Consensus;
class dlg_modalities;
class dlg_progress;
class dlg_samplings;
class dlg_samplingSettings;
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
		dlg_modalities* dlgModalities,
		dlg_labels* dlgLabels,
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
	void StartSampling();
	void SamplingFinished();
	void ClusteringFinished();
	void LoadSampling();
	void LoadClustering();
	void CalculateClustering();
	void saveAll();
	void saveClustering();
	void saveDerivedOutputSlot();
	void DataAvailable();
	void ModalitySelected(int modalityIdx);
	void SetIconSize(int newSize);
	void setColorTheme(const QString &);
	void SetRepresentative(const QString &);
	void LoadRefImg();
	void SetMagicLensCount(int);
	void FreeMemory();
	void SetProbabilityProbing(int);
	void SetCorrectnessUncertainty(int);
	void DataTFChanged();
private:
	void saveGEMSeProject(QString const & fileName, QString const & hiddenCharts);
	void EnableClusteringDependantUI();
	void EnableSamplingDependantUI();
	void saveDerivedOutput(
		QString const & derivedOutputFileName,
		QString const & attributeDescriptorOutputFileName,
		QSharedPointer<iASamplingResults> results);

	
	dlg_modalities*                      m_dlgModalities;
	dlg_samplingSettings*                m_dlgSamplingSettings;
	dlg_progress*						 m_dlgProgress;
	dlg_GEMSe*                           m_dlgGEMSe;
	dlg_labels*                          m_dlgLabels;
	dlg_samplings*                       m_dlgSamplings;
	dlg_Consensus*                       m_dlgConsensus;

	QSharedPointer<iAImageSampler>       m_sampler;
	QSharedPointer<iAImageClusterer>     m_clusterer;

	QString								 m_outputFolder;
	
	QString                              m_cltFile;
	QString                              m_m_metaFileName;
	QSharedPointer<iASimpleLabelInfo>    m_simpleLabelInfo;
	LabelImagePointer                    m_refImg;
	QMap<QString, QString>               m_samplingSettings;
};
