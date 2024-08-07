// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_GEMSe.h"

#include "iAAttributes.h"
#include "iAAttitudes.h"
#include "iACameraWidget.h"
#include "iAClusterAttribChart.h"
#include "iADetailView.h"
#include "iAExampleImageWidget.h"
#include "iAFakeTreeNode.h"
#include "iAFavoriteWidget.h"
#include "iAGEMSeConstants.h"
#include "iAGEMSeScatterplot.h"
#include "iAHistogramContainer.h"
#include "iAImagePreviewWidget.h"
#include "iAImageTreeLeaf.h"
#include "iAImageTreeView.h"
#include "iAMeasures.h"
#include "iAParamHistogramData.h"
#include "iAPreviewWidgetPool.h"
#include "iAProbingWidget.h"
#include "iAQtCaptionWidget.h"
#include "iASamplingResults.h"
#include "iASingleResult.h"

#include <iAAttributeDescriptor.h>
#include <iALog.h>
#include <iAMathUtility.h>
#include <iAToolsITK.h>

#include <vtkImageData.h>

#include <QApplication>

dlg_GEMSe::dlg_GEMSe(
	QWidget *parent,
	iAColorTheme const * colorTheme)
:
	dlg_GEMSeUI(parent),
	m_selectedLeaf(nullptr),
	m_treeView(nullptr),
	m_detailView(nullptr),
	m_exampleView(nullptr),
	m_cameraWidget(nullptr),
	m_favoriteWidget(nullptr),
	m_histogramContainer(nullptr),
	m_scatterplot(nullptr),
	m_probingWidget(nullptr),
	m_colorTheme(colorTheme),
	m_previewWidgetPool(nullptr),
	m_representativeType(iARepresentativeType::Difference)
{
}

void dlg_GEMSe::SetTree(
	std::shared_ptr<iAImageTree> imageTree,
	vtkSmartPointer<vtkImageData> originalImage,
	std::vector<std::shared_ptr<iADataSet>> const& dataSets,
	std::vector<iATransferFunction*> const& transfer,
	iALabelInfo const * labelInfo,
	std::shared_ptr<QVector<std::shared_ptr<iASamplingResults>>> samplings)
{
	// reset previous
	if (m_treeView)
	{
		delete m_histogramContainer;
		delete m_scatterplot;
		delete m_favoriteWidget;
		delete m_detailView;
		delete m_exampleView;
		delete m_cameraWidget;
		delete m_treeView;
		delete m_previewWidgetPool;
	}
	m_samplings = samplings;
	CreateMapper();
	assert(imageTree);
	assert(originalImage);
	if (!imageTree || !originalImage)
	{
		return;
	}
	m_selectedCluster = imageTree->m_root;
	m_selectedLeaf = nullptr;
	m_cameraWidget = new iACameraWidget(this, originalImage, imageTree->labelCount(), iACameraWidget::GridLayout);
	wdCamera->layout()->addWidget(m_cameraWidget);

	m_previewWidgetPool = new iAPreviewWidgetPool(
		MaxPreviewWidgets, m_cameraWidget->commonCamera(), iASlicerMode::XY, imageTree->labelCount(), m_colorTheme);

	m_nullImage = allocateImage(imageTree->m_root->GetRepresentativeImage(iARepresentativeType::Difference,
		LabelImagePointer()));

	m_treeView = new iAImageTreeView(wdTree, imageTree, m_previewWidgetPool, m_representativeType);
	m_treeView->AddSelectedNode(m_selectedCluster, false);

	m_detailView = new iADetailView(m_previewWidgetPool->getWidget(this, true),
		m_previewWidgetPool->getWidget(this, false),
		m_nullImage, dataSets, transfer, *labelInfo,
		m_colorTheme, m_representativeType,
		wdDetails);
	m_detailView->SetNode(m_selectedCluster.get(), m_chartAttributes, m_chartAttributeMapper);
	m_previewWidgetPool->setSliceNumber(m_detailView->sliceNumber());
	wdImagePreview->layout()->addWidget(m_detailView);

	int extent[6];
	originalImage->GetExtent(extent);
	                                        // height             / width
	double aspectRatio = static_cast<double>(extent[3]-extent[2]) / (extent[1]-extent[0]);

	m_exampleView = new iAExampleImageWidget(aspectRatio, m_previewWidgetPool, m_nullImage);
	wdExamples->layout()->addWidget(m_exampleView);
	m_exampleView->SetSelectedNode(m_selectedCluster);

	m_favoriteWidget = new iAFavoriteWidget(m_previewWidgetPool);
	wdFavorites->layout()->addWidget(m_favoriteWidget);

	m_scatterplot = new iAGEMSeScatterplot(wdComparisonCharts);

	if (m_pipelineNames.size() != m_samplings->size())
	{
		LOG(lvlError, "Insufficient number of pipeline names specified!");
		return;
	}
	m_histogramContainer = new iAHistogramContainer(m_chartAttributes, m_chartAttributeMapper, GetRoot().get(), m_pipelineNames);
	wdCharts->layout()->addWidget(m_histogramContainer);
	qobject_cast<QHBoxLayout*>(wdCharts->layout())->setSpacing(ChartSpacing);
	m_histogramContainer->CreateCharts();
	UpdateClusterChartData();

	m_probingWidget = new iAProbingWidget(labelInfo);
	m_probingWidget->SetSelectedNode(m_selectedCluster.get());
	wdProbing->layout()->addWidget(m_probingWidget);

	connect(m_cameraWidget, &iACameraWidget::ModeChanged, this, &dlg_GEMSe::SlicerModeChanged);
	connect(m_treeView, &iAImageTreeView::clicked, this, &dlg_GEMSe::ClusterNodeClicked);
	connect(m_treeView, &iAImageTreeView::ImageClicked, this, &dlg_GEMSe::ClusterNodeImageClicked);
	connect(m_treeView, &iAImageTreeView::ImageRightClicked, this, &dlg_GEMSe::CompareAlternateSelected);
	connect(m_treeView, &iAImageTreeView::Expanded, this, &dlg_GEMSe::SelectCluster);
	connect(m_treeView, &iAImageTreeView::JumpedTo, this, &dlg_GEMSe::SelectCluster);
	connect(m_treeView, &iAImageTreeView::SelectionChanged, this, &dlg_GEMSe::UpdateClusterChartData);
	connect(m_exampleView, &iAExampleImageWidget::Selected, this, &dlg_GEMSe::ClusterLeafSelected);
	connect(m_exampleView, &iAExampleImageWidget::AlternateSelected, this, &dlg_GEMSe::CompareAlternateSelected);
	connect(m_detailView, &iADetailView::Like, this, &dlg_GEMSe::ToggleLike);
	connect(m_detailView, &iADetailView::Hate, this, &dlg_GEMSe::ToggleHate);
	connect(m_detailView, &iADetailView::GoToCluster, this, &dlg_GEMSe::GoToCluster);
	connect(m_detailView, &iADetailView::ResultFilterUpdate, this, &dlg_GEMSe::UpdateResultFilter);
	connect(m_cameraWidget, &iACameraWidget::SliceChanged, this, &dlg_GEMSe::SliceNumberChanged);
	connect(m_favoriteWidget, &iAFavoriteWidget::clicked, this, &dlg_GEMSe::FavoriteClicked);
	connect(m_favoriteWidget, &iAFavoriteWidget::rightClicked, this, &dlg_GEMSe::CompareAlternateSelected);
	connect(m_histogramContainer, &iAHistogramContainer::ChartSelectionUpdated, this, &dlg_GEMSe::HistogramSelectionUpdated);
	connect(m_histogramContainer, QOverload<int,double,double>::of(&iAHistogramContainer::FilterChanged), this, &dlg_GEMSe::FilterChanged);
	connect(m_histogramContainer, QOverload<int>::of(&iAHistogramContainer::ChartDblClicked), this, &dlg_GEMSe::ChartDblClicked);

	// view updates:
	connect(m_detailView,     &iADetailView::ViewUpdated, this, &dlg_GEMSe::UpdateViews);
	connect(m_cameraWidget,   &iACameraWidget::ViewUpdated, this, &dlg_GEMSe::UpdateViews);
	connect(m_treeView,       &iAImageTreeView::ViewUpdated, this, &dlg_GEMSe::UpdateViews);
	connect(m_exampleView,    &iAExampleImageWidget::ViewUpdated, this, &dlg_GEMSe::UpdateViews);
	connect(m_favoriteWidget, &iAFavoriteWidget::ViewUpdated, this, &dlg_GEMSe::UpdateViews);
}

void dlg_GEMSe::CreateMapper()
{
	m_chartAttributes = std::make_shared<iAAttributes>();
	m_chartAttributeMapper.Clear();
	int nextChartID = 0;
	m_pipelineNames.clear();
	for (int samplingIdx=0; samplingIdx<m_samplings->size(); ++samplingIdx)
	{
		std::shared_ptr<iASamplingResults> sampling = m_samplings->at(samplingIdx);
		m_pipelineNames.push_back(sampling->name());
		int datasetID = sampling->id();
		std::shared_ptr<iAAttributes> attributes = sampling->attributes();
		for (int attributeID = 0; attributeID < attributes->size(); ++attributeID)
		{
			int chartID = -1;
			std::shared_ptr<iAAttributeDescriptor> attribute = attributes->at(attributeID);

			// check if previous datasets have an attribute with the same name
			if (samplingIdx > 0 &&
				attribute->attribType() ==
				iAAttributeDescriptor::DerivedOutput) // at the moment for derived output only
			{
				chartID = findAttribute(*m_chartAttributes.get(), attribute->name());
			}
			if (chartID != -1)
			{	// reuse existing chart, only add mapping:
				m_chartAttributeMapper.Add(datasetID, attributeID, chartID);
				// and update min/max:
				m_chartAttributes->at(chartID)->adjustMinMax(attribute->min());
				m_chartAttributes->at(chartID)->adjustMinMax(attribute->max());
			}
			else
			{	// add chart and mapping:
				m_chartAttributes->push_back(attribute);
				chartID = nextChartID;
				nextChartID++;
				m_chartAttributeMapper.Add(datasetID, attributeID, chartID);
			}
		}
	}
	m_MeasureChartIDStart = static_cast<int>(m_chartAttributes->size());
}

void dlg_GEMSe::ClusterNodeClicked(std::shared_ptr<iAImageTreeNode> node)
{
	if (node->GetFilteredSize() == 0)
	{
		return;
	}
	bool wasSelectedBefore = (m_selectedCluster == node);
	m_treeView->UpdateAutoShrink(node.get(), wasSelectedBefore);
	if (!wasSelectedBefore)
	{
		SelectCluster(node);
	}
}

void dlg_GEMSe::ClusterNodeImageClicked(std::shared_ptr<iAImageTreeNode> node)
{
	if (node->GetFilteredSize() == 0)
	{
		return;
	}
	bool wasSelectedBefore = (m_selectedCluster == node);
	if (!wasSelectedBefore)
	{
		SelectCluster(node);
	}
}

void dlg_GEMSe::SelectCluster(std::shared_ptr<iAImageTreeNode> node)
{
	m_selectedCluster = node;
	m_selectedLeaf = nullptr;
	bool clear = !QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
	m_treeView->AddSelectedNode(node, clear);
	m_exampleView->SetSelectedNode(node);
	m_probingWidget->SetSelectedNode(node.get());
	UpdateClusterFilteredChartData();
	m_scatterplot->UpdateClusterPlot(m_selectedCluster.get(), m_chartFilter, m_chartAttributeMapper);
	if (node->IsLeaf())
	{
		iAImageTreeLeaf * leaf = dynamic_cast<iAImageTreeLeaf*>(node.get());
		ClusterLeafSelected(leaf);
		m_scatterplot->UpdateLeafPlot(m_selectedLeaf, m_chartAttributeMapper);
	}
	else
	{
		m_detailView->SetNode(node.get(), m_chartAttributes, m_chartAttributeMapper);
	}
}

void dlg_GEMSe::ClusterLeafSelected(iAImageTreeLeaf * node)
{
	m_selectedLeaf = node;
	m_detailView->SetNode(node, m_chartAttributes, m_chartAttributeMapper);

	for (int chartID=0; chartID<m_chartAttributes->size(); ++chartID)
	{
		if (!m_histogramContainer->ChartExists(chartID))
		{
			continue;
		}
		if (!m_chartAttributeMapper.GetDatasetIDs(chartID).contains(node->GetDatasetID()))
		{
			m_histogramContainer->RemoveMarker(chartID);
			continue;
		}
		int attributeID = m_chartAttributeMapper.GetAttributeID(chartID, node->GetDatasetID());
		double value = node->GetAttribute(attributeID);
		m_histogramContainer->SetMarker(chartID, value);
		m_probingWidget->SetSelectedNode(node);
	}
	m_scatterplot->UpdateLeafPlot(m_selectedLeaf, m_chartAttributeMapper);
}

void dlg_GEMSe::CompareAlternateSelected(iAImageTreeNode * node)
{
	m_detailView->SetCompareNode(node);
}

void dlg_GEMSe::StoreClustering(QString const & fileName)
{
	m_treeView->GetTree()->Store(fileName);
}

void dlg_GEMSe::UpdateClusterChartData()
{
	QVector<std::shared_ptr<iAImageTreeNode> > const selection =  m_treeView->CurrentSelection();
	m_histogramContainer->UpdateClusterChartData(selection);
}

void dlg_GEMSe::HistogramSelectionUpdated()
{
	if (m_histogramContainer->GetSelectedCount() < 2)
	{
		return;
	}
	m_scatterplot->SetDataSource(
		m_histogramContainer->GetSelectedChartID(0),
		m_histogramContainer->GetSelectedChartID(1),
		m_chartAttributes->at(m_histogramContainer->GetSelectedChartID(0))->name(),
		m_chartAttributes->at(m_histogramContainer->GetSelectedChartID(1))->name(),
		m_chartAttributes->at(m_histogramContainer->GetSelectedChartID(0))->isLogScale(),
		m_chartAttributes->at(m_histogramContainer->GetSelectedChartID(1))->isLogScale(),
		m_chartAttributeMapper,
		m_chartFilter,
		GetRoot().get(),
		m_selectedCluster.get(),
		m_selectedLeaf
	);
	if (!m_chartFilter.MatchesAll())
	{
		GetRoot()->ClearFilterData();
	}
}

void dlg_GEMSe::UpdateClusterFilteredChartData()
{
	m_histogramContainer->UpdateClusterFilteredChartData(m_selectedCluster.get(), m_chartFilter);
}

void dlg_GEMSe::UpdateFilteredChartData()
{
	m_histogramContainer->UpdateFilteredChartData(m_chartFilter);
}

void dlg_GEMSe::UpdateFilteredData()
{
	GetRoot()->UpdateFilter(m_chartFilter, m_chartAttributeMapper, m_detailView->GetResultFilter());
	m_treeView->FilterUpdated();
	m_exampleView->FilterUpdated();

	if (m_detailView->IsShowingCluster())
	{
		m_detailView->SetNode(m_selectedCluster.get(), m_chartAttributes, m_chartAttributeMapper);
	}
	UpdateFilteredChartData();
	UpdateClusterChartData();
	UpdateClusterFilteredChartData();
	m_scatterplot->UpdateFilteredAllPlot(GetRoot().get(), m_chartFilter, m_chartAttributeMapper);
	m_scatterplot->UpdateClusterPlot(m_selectedCluster.get(), m_chartFilter, m_chartAttributeMapper);
}

void dlg_GEMSe::FilterChanged(int chartID, double min, double max)
{
	if (m_chartAttributes->at(chartID)->coversWholeRange(min, max))
	{
		m_chartFilter.RemoveFilter(chartID);
	}
	else
	{
		m_chartFilter.AddFilter(chartID, min, max);
	}
	UpdateFilteredData();
}

void dlg_GEMSe::ResetFilters()
{
	if (!m_chartAttributes)
	{
		return;
	}
	m_chartFilter.Reset();
	m_histogramContainer->ResetFilters();
	UpdateFilteredData();
}

void dlg_GEMSe::selectHistograms()
{
	m_histogramContainer->selectHistograms();
	QVector<std::shared_ptr<iAImageTreeNode> > const selection = m_treeView->CurrentSelection();
	// order is important (to get the correct drawing order)
	if (!m_chartFilter.MatchesAll())
	{
		UpdateFilteredChartData();
	}
	m_histogramContainer->UpdateClusterChartData(selection);
	if (!m_chartFilter.MatchesAll())
	{
		UpdateClusterFilteredChartData();
	}
}

void dlg_GEMSe::ToggleHate()
{
	iAImageTreeNode* node = (m_selectedLeaf) ? m_selectedLeaf : m_selectedCluster.get();
	if (!node)
	{
		LOG(lvlError, "ToggleHate: No node selected!");
		return;
	}
	bool isHated = m_favoriteWidget->ToggleHate(node);
	m_detailView->UpdateLikeHate(false, node->GetAttitude() == iAImageTreeNode::Hated);
	m_treeView->UpdateAutoShrink(node, isHated);
	m_treeView->UpdateSubtreeHighlight();
	UpdateAttributeRangeAttitude();
}

void dlg_GEMSe::ToggleLike()
{
	iAImageTreeNode* node = (m_selectedLeaf) ? m_selectedLeaf : m_selectedCluster.get();
	if (!node)
	{
		LOG(lvlError, "ToggleLike: No node selected!");
		return;
	}
	m_favoriteWidget->ToggleLike(node);
	m_detailView->UpdateLikeHate(node->GetAttitude() == iAImageTreeNode::Liked, false);
	m_treeView->UpdateSubtreeHighlight();
	UpdateAttributeRangeAttitude();
}

void dlg_GEMSe::ExportAttributeRangeRanking(QString const & fileName)
{
	m_histogramContainer->ExportAttributeRangeRanking(fileName);
}

void dlg_GEMSe::UpdateAttributeRangeAttitude()
{
	m_histogramContainer->UpdateAttributeRangeAttitude();
}

void dlg_GEMSe::ExportRankings(QString const & fileName)
{
	ExportAttitudesToRankingFile(fileName, GetRoot().get());
}

void dlg_GEMSe::ImportRankings(QString const & fileName)
{
	SetAttitudesFromRankingFile(fileName, GetRoot().get());
	UpdateAttributeRangeAttitude();
	m_treeView->UpdateSubtreeHighlight();
	// TODO: update detail view?
}

void dlg_GEMSe::GetSelection(QVector<std::shared_ptr<iASingleResult> > & result)
{
	m_selectedCluster->GetSelection(result);
}

std::shared_ptr<iAImageTreeNode> dlg_GEMSe::GetSelectedCluster()
{
	return m_selectedCluster;
}

void dlg_GEMSe::AddConsensusImage(iAITKIO::ImagePointer imgData, QString const & name)
{
	std::shared_ptr<iAFakeTreeNode> node(new iAFakeTreeNode(imgData, name));
	m_ConsensusResults.push_back(node);
	m_selectedCluster = node;
	m_detailView->SetNode(node.get(), m_chartAttributes, m_chartAttributeMapper);
	// for the color transfer function:
	if (m_detailView->GetRepresentativeType() != Difference)
	{
		m_detailView->SetRepresentativeType(Difference);
	}
}

void dlg_GEMSe::AddConsensusNumbersImage(iAITKIO::ImagePointer imgData, QString const & name)
{
	std::shared_ptr<iAFakeTreeNode> node(new iAFakeTreeNode(imgData, name));
	m_ConsensusResults.push_back(node);
	m_detailView->SetNode(node.get(), m_chartAttributes, m_chartAttributeMapper);
	// for the color transfer function:
	if (m_detailView->GetRepresentativeType() != AverageEntropy)
	{
		m_detailView->SetRepresentativeType(AverageEntropy);
	}
}

void dlg_GEMSe::JumpToNode(iAImageTreeNode * node, int stepLimit)
{
	if (!node)
	{
		LOG(lvlError, "JumpToNode: No node selected!");
		return;
	}
	if (dynamic_cast<iAFakeTreeNode*>(node) || !m_treeView->JumpToNode(node, stepLimit))
	{
		m_detailView->SetNode(node, m_chartAttributes, m_chartAttributeMapper);
	}
}

void dlg_GEMSe::FavoriteClicked(iAImageTreeNode * node)
{
	JumpToNode(node, 0);
}

void dlg_GEMSe::GoToCluster()
{
	JumpToNode(m_selectedLeaf, 1);
}

void dlg_GEMSe::SliceNumberChanged(int sliceNr)
{
	m_previewWidgetPool->setSliceNumber(sliceNr);
	m_detailView->setSliceNumber(sliceNr);
}

void dlg_GEMSe::SlicerModeChanged(iASlicerMode mode, int sliceNr)
{
	m_previewWidgetPool->setSlicerMode(mode, sliceNr, m_cameraWidget->commonCamera());
}

void dlg_GEMSe::UpdateViews()
{
	m_cameraWidget->updateView();
	m_previewWidgetPool->updateViews();
}

void dlg_GEMSe::ShowImage(vtkSmartPointer<vtkImageData> imgData)
{
	if (!m_cameraWidget)
	{
		LOG(lvlError, "ShowImage: Camera Widget not set!");
		return;
	}
	m_cameraWidget->showImage(imgData);
}

std::shared_ptr<iAImageTreeNode> dlg_GEMSe::GetCurrentCluster()
{
	return m_selectedCluster;
}

std::shared_ptr<iAImageTreeNode> dlg_GEMSe::GetRoot()
{
	return m_treeView->GetTree()->m_root;
}

void dlg_GEMSe::ChartDblClicked(int chartID)
{
	double min, max;
	GetClusterMinMax(m_selectedCluster.get(), chartID, min, max, m_chartAttributeMapper);
	m_histogramContainer->SetSpanValues(chartID, min, max);
}

void dlg_GEMSe::CalculateRefImgComp(std::shared_ptr<iAImageTreeNode> node, LabelImagePointer refImg,
	int labelCount)
{
	if (node->IsLeaf())
	{
		iAImageTreeLeaf * leaf = dynamic_cast<iAImageTreeLeaf*>(node.get());
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(leaf->GetLargeImage().GetPointer());
		QVector<double> measures;
		CalculateMeasures(refImg, lblImg, labelCount, measures);
		// {
		// write measures and parameters to debug out:
		/*
		QString debugOut = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
			.arg(leaf->GetDatasetID())
			.arg(leaf->GetID())
			.arg(measures[0]) // dice
			.arg(measures[2]) // accuracy
			.arg(measures[3]) // precision
			.arg(measures[4]) // recall
			.arg("")		  // undecided
		;
		for (int i = 0; i < leaf->GetAttributes()->size(); ++i)
		{
			if (leaf->GetAttributes()->at(i)->AttribType() == iAAttributeDescriptor::Parameter)
			{
				debugOut += QString("\t%1").arg(leaf->GetAttribute(i));
			}
		}
		LOG(lvlDebug, debugOut);
		*/
		// }
		for (int i=0; i<measures.size(); ++i)
		{
			int chartID = m_MeasureChartIDStart + i;
			int attributeID = m_chartAttributeMapper.GetAttributeID(chartID, leaf->GetDatasetID());
			leaf->SetAttribute(attributeID, measures[i]);
			m_chartAttributes->at(chartID)->adjustMinMax(measures[i]);
		}
	}
	else
	{
		for (int i = 0; i < node->GetChildCount(); ++i)
		{
			CalculateRefImgComp(node->GetChild(i), refImg, labelCount);
		}
	}
}

void dlg_GEMSe::CalcRefImgComp(LabelImagePointer refImg)
{
	if (!refImg)
	{
		LOG(lvlError, "Reference image comparison calculate: nullptr reference image (maybe wrong image type?)!");
		return;
	}
	if (!m_treeView)
	{
		return;
	}
	int labelCount = m_treeView->GetTree()->labelCount();
	m_MeasureChartIDStart = findAttribute(*m_chartAttributes.get(), "Dice");
	if (m_MeasureChartIDStart == -1)
	{
		QVector<std::shared_ptr<iAAttributeDescriptor> > measures;
		measures.push_back(std::make_shared<iAAttributeDescriptor>(
			"Dice", iAAttributeDescriptor::DerivedOutput, iAValueType::Continuous));
		measures.push_back(std::make_shared<iAAttributeDescriptor>(
			"Kappa", iAAttributeDescriptor::DerivedOutput, iAValueType::Continuous));
		measures.push_back(std::make_shared<iAAttributeDescriptor>(
			"Overall Accuracy", iAAttributeDescriptor::DerivedOutput, iAValueType::Continuous));
		measures.push_back(std::make_shared<iAAttributeDescriptor>(
			"Precision", iAAttributeDescriptor::DerivedOutput, iAValueType::Continuous));
		measures.push_back(std::make_shared<iAAttributeDescriptor>(
			"Recall", iAAttributeDescriptor::DerivedOutput, iAValueType::Continuous));
		for (int i=0; i<labelCount; ++i)
		{
			measures.push_back(std::make_shared<iAAttributeDescriptor>(
				QString("Dice %1").arg(i), iAAttributeDescriptor::DerivedOutput, iAValueType::Continuous));
		}
		m_MeasureChartIDStart = static_cast<int>(m_chartAttributes->size());
		for (std::shared_ptr<iAAttributeDescriptor> measure : measures)
		{
			int chartID = static_cast<int>(m_chartAttributes->size());
			m_chartAttributes->push_back(measure);
			// add mappings:
			for (int sampleIdx = 0; sampleIdx < m_samplings->size(); ++sampleIdx)
			{
				std::shared_ptr<iAAttributes> attribs = m_samplings->at(sampleIdx)->attributes();
				int attributeID = static_cast<int>(attribs->size());
				int datasetID = m_samplings->at(sampleIdx)->id();
				attribs->push_back(measure);
				m_chartAttributeMapper.Add(datasetID, attributeID, chartID);
			}
		}
	}
	for (int i= m_MeasureChartIDStart; i<m_MeasureChartIDStart + 5 +labelCount; ++i)
	{
		m_chartAttributes->at(i)->resetMinMax();
	}

	//LOG(lvlInfo, "Measures for ENSEMBLE:");
	CalculateRefImgComp(GetRoot(), refImg, labelCount);	// rewrite using VisitLeafs !
	m_histogramContainer->CreateCharts();
	UpdateClusterChartData();
	m_detailView->SetRefImg(refImg);
	m_treeView->SetRefImg(refImg);
}

void dlg_GEMSe::setColorTheme(iAColorTheme const * colorTheme, iALabelInfo const * labelInfo)
{
	m_colorTheme = colorTheme;
	if (m_previewWidgetPool)
	{
		m_previewWidgetPool->setColorTheme(colorTheme);
	}
	if (m_detailView)
	{
		m_detailView->SetLabelInfo(*labelInfo, colorTheme);
	}
	if (m_probingWidget)
	{
		m_probingWidget->SetLabelInfo(labelInfo);
	}
}


void dlg_GEMSe::ToggleAutoShrink()
{
	if (!m_treeView)
	{
		return;
	}
	m_treeView->SetAutoShrink(!m_treeView->GetAutoShrink());
}

void dlg_GEMSe::SetIconSize(int iconSize)
{
	if (!m_treeView)
	{
		return;
	}
	m_treeView->SetIconSize(iconSize);
}


bool dlg_GEMSe::SetRepresentativeType(int type, LabelImagePointer refImg)
{
	if (!m_treeView)
	{
		return false;
	}
	bool result = m_treeView->SetRepresentativeType(type, refImg);
	if (!result)
	{	// if it failed, reset to what tree view uses
		type = m_treeView->GetRepresentativeType();
	}
	m_detailView->SetRepresentativeType(type);
	return result;
}

void dlg_GEMSe::SetCorrectnessUncertaintyOverlay(bool enabled)
{
	m_detailView->SetCorrectnessUncertaintyOverlay(enabled);
}

int dlg_GEMSe::GetRepresentativeType() const
{
	return m_treeView->GetRepresentativeType();
}

QString dlg_GEMSe::GetSerializedHiddenCharts() const
{
	if (!m_histogramContainer)
	{
		return QString();
	}
	return m_histogramContainer->GetSerializedHiddenCharts();
}

void dlg_GEMSe::SetSerializedHiddenCharts(QString const & hiddenCharts)
{
	if (!m_histogramContainer)
	{
		return;
	}
	m_histogramContainer->SetSerializedHiddenCharts(hiddenCharts);
}

std::shared_ptr<QVector<std::shared_ptr<iASamplingResults>>> dlg_GEMSe::GetSamplings()
{
	return m_samplings;
}

void dlg_GEMSe::setMagicLensCount(int count)
{
	m_detailView->setMagicLensCount(count);
}

void dlg_GEMSe::freeMemory()
{
	if (!m_treeView)
	{
		return;
	}
	m_treeView->freeMemory(GetRoot(), false);
}

void dlg_GEMSe::SetProbabilityProbing(bool enabled)
{
	if (enabled)
	{
		connect(m_detailView, &iADetailView::SlicerHover, m_probingWidget, &iAProbingWidget::ProbeUpdate);
	}
	else
	{
		disconnect(m_detailView, &iADetailView::SlicerHover, m_probingWidget, &iAProbingWidget::ProbeUpdate);
	}
}

void dlg_GEMSe::DataTFChanged()
{
	if (!m_detailView)
	{
		return;
	}
	m_detailView->UpdateMagicLensColors();
}

QString dlg_GEMSe::GetLabelNames() const
{
	return m_detailView ?
		m_detailView->GetLabelNames() :
		QString();
}

void dlg_GEMSe::UpdateResultFilter()
{
	UpdateFilteredData();
}
