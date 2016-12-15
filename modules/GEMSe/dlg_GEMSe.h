/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "ui_GEMSe.h"
#include <iAQTtoUIConnector.h>
typedef iAQTtoUIConnector<QDockWidget, Ui_GEMSe> dlg_GEMSeUI;

#include "iAChartAttributeMapper.h"
#include "iAChartFilter.h"
#include "iAGEMSeConstants.h"
#include "iAImageTreeNode.h"
#include "iASlicerMode.h"

#include <vtkSmartPointer.h>

#include <QVector>

class iAAttributes;
class iACameraWidget;
class iAColorTheme;
class iADetailView;
class iAFakeTreeLeaf;
class iAFavoriteWidget;
class iAGEMSeScatterplot;
class iAHistogramContainer;
class iAImageTree;
class iAImageTreeLeaf;
class iAImageTreeView;
class iAExampleImageWidget;
class iALabelInfo;
class iALogger;
class iAModalityList;
class iAPreviewWidgetPool;
class iASamplingResults;
class iASingleResult;

class vtkImageData;

class dlg_GEMSe: public dlg_GEMSeUI
{
	Q_OBJECT
public:
	dlg_GEMSe(QWidget *parent, iALogger * logger, iAColorTheme const * colorTheme);
	void SetTree(QSharedPointer<iAImageTree> imageTree,
		vtkSmartPointer<vtkImageData> originalImage,
		QSharedPointer<iAModalityList> modalities,
		iALabelInfo const & labelInfo,
		QSharedPointer<QVector<QSharedPointer<iASamplingResults> > > samplings);
	void StoreClustering(QString const & fileName);
	QSharedPointer<iAImageTreeNode> GetCurrentCluster();
	QSharedPointer<iAImageTreeNode> GetRoot();
	void SetColorTheme(iAColorTheme const * colorTheme, iALabelInfo const& labelInfo);
	void ShowImage(vtkSmartPointer<vtkImageData> imgData);
	void CalcRefImgComp(LabelImagePointer refImg);
	void ToggleAutoShrink();
	void SetMagicLensOpacity(double opacity);
	void SetIconSize(int iconSize);
	bool SetRepresentativeType(int type);
	int GetRepresentativeType() const;
	void ExportAttributeRangeRanking(QString const & fileName);
	void ExportRankings(QString const & fileName);
	void ImportRankings(QString const & fileName);
	void GetSelection(QVector<QSharedPointer<iASingleResult> > &);
	QSharedPointer<iAImageTreeNode> GetSelectedCluster();
	void AddMajorityVotingImage(iAITKIO::ImagePointer imgData);
	void AddMajorityVotingNumbers(iAITKIO::ImagePointer imgData);
	int GetMeasureStartID() { return m_MeasureChartIDStart; }
	QString GetSerializedHiddenCharts() const;
	void SetSerializedHiddenCharts(QString const & hiddenCharts);
	QSharedPointer<QVector<QSharedPointer<iASamplingResults> > > GetSamplings();
public slots:
	void ResetFilters();
	void SelectHistograms();
private slots:
	void ClusterNodeClicked(QSharedPointer<iAImageTreeNode> node);
	void ClusterNodeImageClicked(QSharedPointer<iAImageTreeNode> node);
	void SelectCluster(QSharedPointer<iAImageTreeNode> node);
	void ClusterLeafSelected(iAImageTreeLeaf *);
	void FilterChanged(int chartID, double min, double max);
	void ChartDblClicked(int chartID);
	void ToggleHate();
	void ToggleLike();
	void GoToCluster();
	void FavoriteClicked(iAImageTreeNode * leaf);
	
	void SlicerModeChanged(iASlicerMode mode, int sliceNr);
	void SliceNumberChanged(int sliceNr);
	void UpdateViews();
	void UpdateClusterChartData();
	void HistogramSelectionUpdated();
private:
	void JumpToNode(iAImageTreeNode * leaf, int stepLimit);
	void UpdateFilteredChartData();
	void UpdateClusterFilteredChartData();
	void UpdateFilteredData();
	void UpdateAttributeRangeAttitude();
	void CreateMapper();
	void CalculateRefImgComp(QSharedPointer<iAImageTreeNode> node, LabelImagePointer refImg,
		int labelCount);
	
	// data:
	QSharedPointer<QVector<QSharedPointer<iASamplingResults> > > m_samplings;
	QStringList m_pipelineNames;
	QSharedPointer<iAAttributes> m_chartAttributes;
	iAChartAttributeMapper m_chartAttributeMapper;
	int m_MeasureChartIDStart;
	int m_MajorityVotingSamplingID;
	int m_MajorityVotingID;
	QVector<QSharedPointer<iAImageTreeLeaf> > m_majorityVotingLeafs;
	
	QSharedPointer<iAImageTreeNode> m_selectedCluster;
	iAImageTreeLeaf * m_selectedLeaf;

	iAChartFilter m_chartFilter;

	// GUI components:
	iAImageTreeView* m_treeView;
	iADetailView* m_detailView;
	iAExampleImageWidget * m_exampleView;
	iACameraWidget* m_cameraWidget;
	iAFavoriteWidget* m_favoriteWidget;
	iAColorTheme const * m_colorTheme;
	iAHistogramContainer * m_histogramContainer;
	iAGEMSeScatterplot * m_scatterplot;

	iALogger* m_logger;
	iAPreviewWidgetPool* m_previewWidgetPool;
	ClusterImageType m_nullImage;

	iARepresentativeType m_representativeType;
	QSharedPointer<iAFakeTreeLeaf> m_currentMajorityVotingResult;
};
