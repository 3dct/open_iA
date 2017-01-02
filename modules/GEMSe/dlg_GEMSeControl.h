/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include <vtkSmartPointer.h>

#include <iAQTtoUIConnector.h>
typedef iAQTtoUIConnector<QDockWidget, Ui_GEMSeControl>   dlg_GEMSeControlUI;

class iAImageClusterer;
class iAImageSampler;

class dlg_GEMSe;
class dlg_labels;
class dlg_MajorityVoting;
class dlg_modalities;
class dlg_progress;
class dlg_samplings;
class dlg_samplingSettings;
class iAColorTheme;
class iASimpleLabelInfo;
class iASamplingResults;

class vtkImageData;

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
	bool LoadSampling(QString const & fileName, int labelCount, int datasetID);
	bool LoadClustering(QString const & fileName);
	bool LoadReferenceImage(QString const & referenceImageName);
	void ExportAttributeRangeRanking();
	void ExportRankings();
	void ImportRankings();
	void SetSerializedHiddenCharts(QString const & hiddenCharts);
public slots:
	void ExportIDs();
private slots:
	void StartSampling();
	void SamplingFinished();
	void ClusteringFinished();
	void LoadSampling();
	void LoadClustering();
	void CalculateClustering();
	void StoreClustering();
	void StoreAll();
	void DataAvailable();
	void ShowImage(vtkSmartPointer<vtkImageData> imgData);
	void SetIconSize(int newSize);
	void SetColorTheme(const QString &);
	void SetRepresentative(const QString &);
	void LoadRefImage();
	void StoreDerivedOutput();
	void SamplingAdded(QSharedPointer<iASamplingResults>);
	void SetMagicLensCount(int);
private:
	void StoreGEMSeProject(QString const & fileName, QString const & hiddenCharts);
	void EnableClusteringDependantUI();
	void EnableSamplingDependantUI();
	void StoreDerivedOutput(
		QString const & derivedOutputFileName,
		QString const & attributeDescriptorOutputFileName,
		QSharedPointer<iASamplingResults> results);

	
	dlg_modalities*                      m_dlgModalities;
	dlg_samplingSettings*                m_dlgSamplingSettings;
	dlg_progress*						 m_dlgProgress;
	dlg_GEMSe*                           m_dlgGEMSe;
	dlg_labels*                          m_dlgLabels;
	dlg_samplings*                       m_dlgSamplings;
	dlg_MajorityVoting*                  m_dlgMajorityVoting;

	QSharedPointer<iAImageSampler>       m_sampler;
	QSharedPointer<iAImageClusterer>     m_clusterer;

	QString								 m_outputFolder;
	
	QString                              m_cltFile;
	QString                              m_m_metaFileName;
	QSharedPointer<iASimpleLabelInfo>    m_simpleLabelInfo;
	LabelImagePointer                    m_groundTruthImage;
};
