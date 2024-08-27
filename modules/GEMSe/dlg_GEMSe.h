// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_GEMSe.h"

#include "iAChartAttributeMapper.h"
#include "iAChartFilter.h"
#include "iAGEMSeConstants.h"
#include "iAImageTreeNode.h"

#include <iAAttributes.h>
#include <iASlicerMode.h>
#include <iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QVector>

class iACameraWidget;
class iAColorTheme;
class iADataSet;
class iADetailView;
class iAFakeTreeNode;
class iAFavoriteWidget;
class iAGEMSeScatterplot;
class iAHistogramContainer;
class iAImageTree;
class iAImageTreeLeaf;
class iAImageTreeView;
class iAExampleImageWidget;
class iALabelInfo;
class iAPreviewWidgetPool;
class iAProbingWidget;
class iASamplingResults;
class iASingleResult;
class iATransferFunction;

class vtkImageData;

typedef iAQTtoUIConnector<QDockWidget, Ui_GEMSe> dlg_GEMSeUI;

class dlg_GEMSe: public dlg_GEMSeUI
{
	Q_OBJECT
public:
	dlg_GEMSe(QWidget *parent, iAColorTheme const * colorTheme);
	void SetTree(std::shared_ptr<iAImageTree> imageTree,
		vtkSmartPointer<vtkImageData> originalImage,
		std::vector<std::shared_ptr<iADataSet>> const& dataSets,
		std::vector<iATransferFunction*> const& transfer,
		iALabelInfo const * labelInfo,
		std::shared_ptr<QVector<std::shared_ptr<iASamplingResults>>> samplings);
	void StoreClustering(QString const & fileName);
	std::shared_ptr<iAImageTreeNode> GetCurrentCluster();
	std::shared_ptr<iAImageTreeNode> GetRoot();
	void setColorTheme(iAColorTheme const * colorTheme, iALabelInfo const * labelInfo);
	void ShowImage(vtkSmartPointer<vtkImageData> imgData);
	void CalcRefImgComp(LabelImagePointer refImg);
	void ToggleAutoShrink();
	void SetIconSize(int iconSize);
	bool SetRepresentativeType(int type, LabelImagePointer refImg);
	void SetCorrectnessUncertaintyOverlay(bool enabled);
	int GetRepresentativeType() const;
	void ExportAttributeRangeRanking(QString const & fileName);
	void ExportRankings(QString const & fileName);
	void ImportRankings(QString const & fileName);
	void GetSelection(QVector<std::shared_ptr<iASingleResult> > &);
	std::shared_ptr<iAImageTreeNode> GetSelectedCluster();
	void AddConsensusImage(iAITKIO::ImagePointer imgData, QString const & name);
	void AddConsensusNumbersImage(iAITKIO::ImagePointer imgData, QString const & name);
	int GetMeasureStartID() { return m_MeasureChartIDStart; }
	QString GetSerializedHiddenCharts() const;
	void SetSerializedHiddenCharts(QString const & hiddenCharts);
	std::shared_ptr<QVector<std::shared_ptr<iASamplingResults>>> GetSamplings();
	void setMagicLensCount(int count);
	void freeMemory();
	void SetProbabilityProbing(bool enabled);
	void DataTFChanged();
	QString GetLabelNames() const;
public slots:
	void ResetFilters();
	void selectHistograms();
private slots:
	void ClusterNodeClicked(std::shared_ptr<iAImageTreeNode> node);
	void ClusterNodeImageClicked(std::shared_ptr<iAImageTreeNode> node);
	void SelectCluster(std::shared_ptr<iAImageTreeNode> node);
	void ClusterLeafSelected(iAImageTreeLeaf *);
	void CompareAlternateSelected(iAImageTreeNode * node);
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
	void UpdateResultFilter();
private:
	void JumpToNode(iAImageTreeNode * leaf, int stepLimit);
	void UpdateFilteredChartData();
	void UpdateClusterFilteredChartData();
	void UpdateFilteredData();
	void UpdateAttributeRangeAttitude();
	void CreateMapper();
	void CalculateRefImgComp(std::shared_ptr<iAImageTreeNode> node, LabelImagePointer refImg,
		int labelCount);

	// data:
	std::shared_ptr<QVector<std::shared_ptr<iASamplingResults>>> m_samplings;
	QStringList m_pipelineNames;
	std::shared_ptr<iAAttributes> m_chartAttributes;
	iAChartAttributeMapper m_chartAttributeMapper;
	int m_MeasureChartIDStart;

	std::shared_ptr<iAImageTreeNode> m_selectedCluster;
	iAImageTreeLeaf * m_selectedLeaf;

	iAChartFilter m_chartFilter;

	// GUI components:
	iAImageTreeView* m_treeView;
	iADetailView* m_detailView;
	iAExampleImageWidget * m_exampleView;
	iACameraWidget* m_cameraWidget;
	iAFavoriteWidget* m_favoriteWidget;
	iAHistogramContainer * m_histogramContainer;
	iAGEMSeScatterplot * m_scatterplot;
	iAProbingWidget * m_probingWidget;
	iAColorTheme const* m_colorTheme;
	iAPreviewWidgetPool* m_previewWidgetPool;
	ClusterImageType m_nullImage;
	iARepresentativeType m_representativeType;
	QVector<std::shared_ptr<iAFakeTreeNode> > m_ConsensusResults;		// to store them somewhere so they don't get deleted - TODO: discard once unused!
};
