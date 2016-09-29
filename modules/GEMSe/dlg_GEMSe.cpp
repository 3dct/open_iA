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
 
#include "pch.h"
#include "dlg_GEMSe.h"

#include "iAAttributes.h"
#include "iAAttitudes.h"
#include "iAAttributeDescriptor.h"
#include "iACameraWidget.h"
#include "iAChartSpanSlider.h"
#include "iAConsole.h"
#include "iADetailView.h"
#include "iAExampleImageWidget.h"
#include "iAFavoriteWidget.h"
#include "iAGEMSeConstants.h"
#include "iAHistogramContainer.h"
#include "iAImagePreviewWidget.h"
#include "iAImageTreeView.h"
#include "iALogger.h"
#include "iAMathUtility.h"
#include "iAMeasures.h"
#include "iAParamHistogramData.h"
#include "iAPreviewWidgetPool.h"
#include "iAQtCaptionWidget.h"
#include "iASamplingResults.h"
#include "iAToolsITK.h"

#include <QVTKWidget2.h>
#include <vtkAxis.h>
#include <vtkBrush.h>
#include <vtkChartXY.h>
#include <vtkContextView.h>
#include <vtkContextScene.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPlot.h>
#include <vtkTable.h>

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>

dlg_GEMSe::dlg_GEMSe(
	QWidget *parent,
	iALogger * logger,
	iAColorTheme const * colorTheme)
:
	dlg_GEMSeUI(parent),
	m_singlePlot(0),
	m_clusterPlot(0),
	m_selectedCluster(0),
	m_treeView(0),
	m_detailView(0),
	m_logger(logger),
	m_cameraWidget(0),
	m_exampleView(0),
	m_favoriteWidget(0),
	m_selectedLeaf(0),
	m_previewWidgetPool(0),
	m_colorTheme(colorTheme),
	m_representativeType(iARepresentativeType::Difference)
{
}

dlg_GEMSe::~dlg_GEMSe()
{
}

void dlg_GEMSe::SetTree(
	QSharedPointer<iAImageTree > imageTree,
	vtkSmartPointer<vtkImageData> originalImage,
	QSharedPointer<iAModalityList> modalities,
	iALabelInfo const & labelInfo,
	QVector<QSharedPointer<iASamplingResults> > samplings)
{
	// reset previous
	if (m_treeView)
	{
		delete m_histogramContainer;
		delete m_scatterplotWidget;
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
	m_selectedLeaf = 0;
	m_cameraWidget = new iACameraWidget(this, originalImage, imageTree->GetLabelCount(), iACameraWidget::GridLayout);
	wdCamera->layout()->addWidget(m_cameraWidget);

	m_previewWidgetPool = new iAPreviewWidgetPool(
		MaxPreviewWidgets, m_cameraWidget->GetCommonCamera(), iASlicerMode::XY, imageTree->GetLabelCount(), m_colorTheme);

	m_nullImage = AllocateImage(imageTree->m_root->GetRepresentativeImage(iARepresentativeType::Difference));

	m_treeView = new iAImageTreeView(wdTree, imageTree, m_previewWidgetPool, m_representativeType);
	m_treeView->EnableSubtreeHighlight(true);
	m_treeView->AddSelectedNode(m_selectedCluster, false);
	wdTree->layout()->addWidget(m_treeView);

	m_detailView = new iADetailView(m_previewWidgetPool->GetWidget(this, true), m_nullImage, modalities, labelInfo,
		m_representativeType);
	m_detailView->SetNode(m_selectedCluster.data(), m_chartAttributes, m_chartAttributeMapper);
	m_previewWidgetPool->SetSliceNumber(m_detailView->GetSliceNumber());
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
	wdFavorites->hide();

	m_scatterplotWidget = new QVTKWidget2();
	vtkSmartPointer<vtkContextView> contextView(vtkSmartPointer<vtkContextView>::New());
	contextView->SetRenderWindow(m_scatterplotWidget->GetRenderWindow());
	m_comparisonChart = vtkSmartPointer<vtkChartXY>::New();
	m_comparisonChart->SetSelectionMode(vtkContextScene::SELECTION_NONE);
	contextView->GetScene()->AddItem(m_comparisonChart);
	wdComparisonCharts->layout()->addWidget(m_scatterplotWidget);
	wdComparisonCharts->hide();

	m_histogramContainer = new iAHistogramContainer();
	wdCharts->layout()->addWidget(m_histogramContainer);
	dynamic_cast<QHBoxLayout*>(wdCharts->layout())->setSpacing(ChartSpacing);
	m_histogramContainer->CreateCharts(m_chartAttributes, m_chartAttributeMapper, m_treeView->GetTree()->m_root.data());
	UpdateClusterChartData();

	connect(m_cameraWidget, SIGNAL(ModeChanged(iASlicerMode, int)), this, SLOT(SlicerModeChanged(iASlicerMode, int)));
	connect(m_treeView, SIGNAL(Clicked(QSharedPointer<iAImageTreeNode>)), this, SLOT(ClusterNodeClicked(QSharedPointer<iAImageTreeNode>)));
	connect(m_treeView, SIGNAL(ImageClicked(QSharedPointer<iAImageTreeNode>)), this, SLOT(ClusterNodeImageClicked(QSharedPointer<iAImageTreeNode>)));
	connect(m_treeView, SIGNAL(Expanded(QSharedPointer<iAImageTreeNode>)), this, SLOT(SelectCluster(QSharedPointer<iAImageTreeNode>)));
	connect(m_treeView, SIGNAL(JumpedTo(QSharedPointer<iAImageTreeNode>)), this, SLOT(SelectCluster(QSharedPointer<iAImageTreeNode>)));
	connect(m_treeView, SIGNAL(SelectionChanged()), this, SLOT(UpdateClusterChartData()));
	connect(m_exampleView, SIGNAL(Selected(iAImageTreeLeaf *)), this, SLOT(ClusterLeafSelected(iAImageTreeLeaf *)));
	connect(m_detailView, SIGNAL(Like()), this, SLOT(ToggleLike()));
	connect(m_detailView, SIGNAL(Hate()), this, SLOT(ToggleHate()));
	connect(m_detailView, SIGNAL(GoToCluster()), this, SLOT(GoToCluster()));
	connect(m_cameraWidget, SIGNAL(SliceChanged(int)), this, SLOT(SliceNumberChanged(int)));
	connect(m_favoriteWidget, SIGNAL(Clicked(iAImageTreeNode *)), this, SLOT(FavoriteClicked(iAImageTreeNode *)));
	connect(m_histogramContainer, SIGNAL(ChartSelectionUpdated()), this, SLOT(HistogramSelectionUpdated()));
	connect(m_histogramContainer, SIGNAL(FilterChanged(int, double, double)), this, SLOT(FilterChanged(int, double, double)));
	connect(m_histogramContainer, SIGNAL(ChartDblClicked(int)), this, SLOT(ChartDblClicked(int)));

	// view updates:
	connect(m_detailView,     SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );
	connect(m_cameraWidget,   SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );
	connect(m_treeView,       SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );
	connect(m_exampleView,    SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );
	connect(m_favoriteWidget, SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );
}


void dlg_GEMSe::UpdateScatterPlot()
{
	iAChartFilter emptyFilter;
	m_allTable = GetComparisonTable(m_treeView->GetTree()->m_root.data(), emptyFilter); // TODO do we need m_comparisonTable as member?
	if (!m_allTable)
	{
		wdComparisonCharts->hide();
		return;
	}
	if (m_histogramContainer->GetSelectedCount() < 2)
	{
		DEBUG_LOG("You must select two histograms");
		return;
	}
	m_comparisonChart->ClearPlots();
	m_singlePlot = 0;
	m_clusterPlot = 0;
	auto xAxis = m_comparisonChart->GetAxis(vtkAxis::BOTTOM);
	auto yAxis = m_comparisonChart->GetAxis(vtkAxis::LEFT);
	int idx1 = m_histogramContainer->GetSelectedChartID(0);
	int idx2 = m_histogramContainer->GetSelectedChartID(1);
	xAxis->SetTitle(m_chartAttributes->at(idx1)->GetName().toStdString());
	xAxis->SetLogScale(m_chartAttributes->at(idx1)->IsLogScale() );
	yAxis->SetTitle(m_chartAttributes->at(idx2)->GetName().toStdString());
	yAxis->SetLogScale(m_chartAttributes->at(idx2)->IsLogScale() );

	vtkPlot* plot = m_comparisonChart->AddPlot(vtkChart::POINTS);
	plot->SetColor(
		static_cast<unsigned char>(DefaultColors::AllDataChartColor.red()),
		static_cast<unsigned char>(DefaultColors::AllDataChartColor.green()),
		static_cast<unsigned char>(DefaultColors::AllDataChartColor.blue()),
		static_cast<unsigned char>(DefaultColors::AllDataChartColor.alpha())
	);
	plot->SetWidth(1.0);
	plot->SetInputData(m_allTable, 0, 1);
	UpdateScatterPlotClusterPlot();
	UpdateScatterPlotLeafPlot();
	if (!m_chartFilter.MatchesAll())
	{
		UpdateScatterPlot(m_allFilteredTable, m_allFilteredPlot, m_treeView->GetTree()->m_root.data(), DefaultColors::FilteredChartColor, m_chartFilter);
		m_treeView->GetTree()->m_root->ClearFilterData();
	}
}

void dlg_GEMSe::UpdateScatterPlot(vtkSmartPointer<vtkTable> & table, vtkSmartPointer<vtkPlot> & plot, iAImageTreeNode const * node,
	QColor const & color, iAChartFilter const & filter)
{
	if (plot)
	{
		m_comparisonChart->RemovePlotInstance(plot);
		plot = 0;
	}
	if (!node)
	{
		return;
	}
	table = GetComparisonTable(node, filter);
	if (!table)
	{
		return;
	}
	plot = m_comparisonChart->AddPlot(vtkChart::POINTS);
	// TODO: set plot order? at the moment this is done only implicitly by callingt AddPlot in the right order
	
	plot->SetColor(
		static_cast<unsigned char>(color.red()),
		static_cast<unsigned char>(color.green()),
		static_cast<unsigned char>(color.blue()),
		static_cast<unsigned char>(color.alpha())
	);
	plot->SetInputData(table, 0, 1);
	wdComparisonCharts->show();
}

void dlg_GEMSe::UpdateScatterPlotLeafPlot()
{
	iAChartFilter emptyFilter;
	UpdateScatterPlot(m_singleTable, m_singlePlot, m_selectedLeaf, DefaultColors::ImageChartColor, emptyFilter);
}

void dlg_GEMSe::UpdateScatterPlotClusterPlot()
{
	iAChartFilter emptyFilter;
	UpdateScatterPlot(m_clusterTable, m_clusterPlot, m_selectedCluster.data(), DefaultColors::ClusterChartColor[0], emptyFilter);
	if (!m_chartFilter.MatchesAll())
	{
		UpdateScatterPlot(m_clusterFilteredTable, m_clusterFilteredPlot, m_selectedCluster.data(), DefaultColors::FilteredClusterChartColor, m_chartFilter);
	}
}


void dlg_GEMSe::CreateMapper()
{
	m_chartAttributes = QSharedPointer<iAAttributes>(new iAAttributes());
	m_chartAttributeMapper.Clear();
	int nextChartID = 0;
	for (int samplingIdx=0; samplingIdx<m_samplings.size(); ++samplingIdx)
	{
		QSharedPointer<iASamplingResults> sampling = m_samplings[samplingIdx];
		int datasetID = sampling->GetID();
		QSharedPointer<iAAttributes> attributes = sampling->GetAttributes();
		for (int attributeID = 0; attributeID < attributes->size(); ++attributeID)
		{
			int chartID = -1;
			QSharedPointer<iAAttributeDescriptor> attribute = attributes->at(attributeID);
			
			// check if previous datasets have an attribute with the same name
			if (samplingIdx > 0 &&
				attribute->GetAttribType() ==
				iAAttributeDescriptor::DerivedOutput) // at the moment for derived output only
			{
				chartID = m_chartAttributes->Find(attribute->GetName());
			}
			if (chartID != -1)
			{	// reuse existing chart, only add mapping:
				m_chartAttributeMapper.Add(datasetID, attributeID, chartID);
			}
			else
			{	// add chart and mapping:
				m_chartAttributes->Add(attribute);
				chartID = nextChartID;
				nextChartID++;
				m_chartAttributeMapper.Add(datasetID, attributeID, chartID);
			}
		}
	}
	m_MeasureChartIDStart = m_chartAttributes->size();
}

void AddTableValues(vtkSmartPointer<vtkTable> table, iAImageTreeNode const * node, int p1, int p2, int & rowNr,
	iAChartFilter const & attribFilter,
	iAChartAttributeMapper const & chartAttrMap)
{
	if (node->IsLeaf())
	{
		iAImageTreeLeaf const * leaf = dynamic_cast<iAImageTreeLeaf const *>(node);
		if (attribFilter.Matches(leaf, chartAttrMap))
		{
			int attr1Idx = chartAttrMap.GetAttributeID(p1, leaf->GetDatasetID());
			int attr2Idx = chartAttrMap.GetAttributeID(p2, leaf->GetDatasetID());
			double attr1Value = node->GetAttribute(attr1Idx);
			double attr2Value = node->GetAttribute(attr2Idx);
			table->SetValue(rowNr, 0, attr1Value);
			table->SetValue(rowNr, 1, attr2Value);
			rowNr++;
		}
	}
	else
	{
		for (int i=0; i<node->GetChildCount(); ++i)
		{
			AddTableValues(table, node->GetChild(i).data(), p1, p2, rowNr, attribFilter, chartAttrMap);
		}
	}
}

vtkSmartPointer<vtkTable> dlg_GEMSe::GetComparisonTable(iAImageTreeNode const * node, iAChartFilter const & attribFilter)
{
	if (!node || m_histogramContainer->GetSelectedCount() < 2)
	{
		return vtkSmartPointer<vtkTable>();
	}
	vtkSmartPointer<vtkTable> comparisonTable = vtkSmartPointer<vtkTable>::New();

	int numberOfSamples = node->GetClusterSize();
	vtkSmartPointer<vtkFloatArray> arrX(vtkSmartPointer<vtkFloatArray>::New());
	vtkSmartPointer<vtkFloatArray> arrY(vtkSmartPointer<vtkFloatArray>::New());
	arrX->SetName(m_chartAttributes->at(m_histogramContainer->GetSelectedChartID(0))->GetName().toStdString().c_str());  // TODO: get
	arrY->SetName(m_chartAttributes->at(m_histogramContainer->GetSelectedChartID(1))->GetName().toStdString().c_str());
	comparisonTable->AddColumn(arrX);
	comparisonTable->AddColumn(arrY);
	// we may call SetNumberOfRows only after we've added all columns, otherwise there is an error
	comparisonTable->SetNumberOfRows(numberOfSamples);

	int rowNr = 0;
	AddTableValues(comparisonTable, node, m_histogramContainer->GetSelectedChartID(0), m_histogramContainer->GetSelectedChartID(1), rowNr, attribFilter,
		m_chartAttributeMapper); 

	// reduce number of rows to actual number:
	comparisonTable->SetNumberOfRows(rowNr);
	return comparisonTable;
}

void dlg_GEMSe::ClusterNodeClicked(QSharedPointer<iAImageTreeNode> node)
{
	if (node->GetFilteredSize() == 0)
	{
		return;
	}
	bool wasSelectedBefore = (m_selectedCluster == node);
	m_treeView->UpdateAutoShrink(node.data(), wasSelectedBefore);
	if (!wasSelectedBefore)
	{
		SelectCluster(node);
	}
}


void dlg_GEMSe::ClusterNodeImageClicked(QSharedPointer<iAImageTreeNode> node)
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

void dlg_GEMSe::SelectCluster(QSharedPointer<iAImageTreeNode> node)
{
	m_selectedCluster = node;
	m_selectedLeaf = 0;
	bool clear = !QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
	m_treeView->AddSelectedNode(node, clear);
	m_exampleView->SetSelectedNode(node);
	UpdateClusterFilteredChartData();
	UpdateScatterPlotClusterPlot();
	UpdateScatterPlotLeafPlot();
	if (node->IsLeaf())
	{
		iAImageTreeLeaf * leaf = dynamic_cast<iAImageTreeLeaf*>(node.data());
		ClusterLeafSelected(leaf);
	}
	else
	{
		m_detailView->SetNode(node.data(), m_chartAttributes, m_chartAttributeMapper);
	}
}


void dlg_GEMSe::ClusterLeafSelected(iAImageTreeLeaf * node)
{
	m_selectedLeaf = node;
	m_detailView->SetNode(node, m_chartAttributes, m_chartAttributeMapper);

	for (int chartID=0; chartID<m_chartAttributes->size(); ++chartID)
	{
		if (m_histogramContainer->ChartExists(chartID))
		{
			continue;
		}
		if (!m_chartAttributeMapper.GetDatasetIDs(chartID).contains(node->GetDatasetID()))
		{
			continue;
		}
		int attributeID = m_chartAttributeMapper.GetAttributeID(chartID, node->GetDatasetID());
		double value = node->GetAttribute(attributeID);
		if (m_chartAttributes->at(chartID)->GetValueType() == Discrete ||
			m_chartAttributes->at(chartID)->GetValueType() == Categorical)
		{
			value += 0.5;
		}
		m_histogramContainer->SetMarker(chartID, value);
	}
	UpdateScatterPlotLeafPlot();
}

void dlg_GEMSe::StoreClustering(QString const & fileName)
{
	m_treeView->GetTree()->Store(fileName);
}

void dlg_GEMSe::UpdateClusterChartData()
{
	QVector<QSharedPointer<iAImageTreeNode> > const selection =  m_treeView->CurrentSelection();
	m_histogramContainer->UpdateClusterChartData(m_chartAttributes, m_chartAttributeMapper, selection);
}

void dlg_GEMSe::HistogramSelectionUpdated()
{
	UpdateScatterPlot();
}

void dlg_GEMSe::UpdateClusterFilteredChartData()
{
	m_histogramContainer->UpdateClusterFilteredChartData(m_chartAttributes, m_chartAttributeMapper, m_selectedCluster.data(), m_chartFilter);
}

void dlg_GEMSe::UpdateFilteredChartData()
{
	m_histogramContainer->UpdateFilteredChartData(m_chartAttributes, m_chartAttributeMapper, m_treeView->GetTree()->m_root.data(), m_chartFilter);
}


void dlg_GEMSe::UpdateFilteredData()
{
	m_treeView->GetTree()->m_root->UpdateFilter(m_chartFilter, m_chartAttributeMapper);
	m_treeView->FilterUpdated();
	m_exampleView->FilterUpdated();

	if (m_detailView->IsShowingCluster())
	{
		m_detailView->SetNode(m_selectedCluster.data(), m_chartAttributes, m_chartAttributeMapper);
	}
	UpdateFilteredChartData();
	UpdateClusterFilteredChartData();
	if (!m_chartFilter.MatchesAll())
	{
		UpdateScatterPlot(m_allFilteredTable, m_allFilteredPlot, m_treeView->GetTree()->m_root.data(), DefaultColors::FilteredChartColor, m_chartFilter);
	}
	UpdateScatterPlotClusterPlot();
}

void dlg_GEMSe::FilterChanged(int chartID, double min, double max)
{
	if (m_chartAttributes->at(chartID)->CoversWholeRange(min, max))
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
	m_histogramContainer->ResetFilters(m_chartAttributes);
	UpdateFilteredData();
}


void dlg_GEMSe::ToggleHate()
{
	iAImageTreeNode* node = (m_selectedLeaf) ? m_selectedLeaf : m_selectedCluster.data();
	if (!node)
	{
		m_logger->log("ToggleHate No node selected!");
		return;
	}
	bool isHated = m_favoriteWidget->ToggleHate(node);
	m_detailView->UpdateLikeHate(false, node->GetAttitude() == iAImageTreeNode::Hated);
	m_treeView->UpdateAutoShrink(node, isHated);
	m_treeView->UpdateSubtreeHighlight();

	wdFavorites->setVisible(m_favoriteWidget->HasAnyFavorite());

	UpdateAttributeRangeAttitude();
}


void dlg_GEMSe::ToggleLike()
{
	iAImageTreeNode* node = (m_selectedLeaf) ? m_selectedLeaf : m_selectedCluster.data();
	if (!node)
	{
		m_logger->log("ToggleHate No node selected!");
		return;
	}
	m_favoriteWidget->ToggleLike(node);
	m_detailView->UpdateLikeHate(node->GetAttitude() == iAImageTreeNode::Liked, false);
	m_treeView->UpdateSubtreeHighlight();

	wdFavorites->setVisible(m_favoriteWidget->HasAnyFavorite());
	UpdateAttributeRangeAttitude();
}

void dlg_GEMSe::ExportAttributeRangeRanking(QString const & fileName)
{
	m_histogramContainer->ExportAttributeRangeRanking(fileName, m_chartAttributes);
}

void dlg_GEMSe::UpdateAttributeRangeAttitude()
{
	m_histogramContainer->UpdateAttributeRangeAttitude(m_chartAttributes, m_chartAttributeMapper, m_treeView->GetTree()->m_root.data());
}

void dlg_GEMSe::ExportRankings(QString const & fileName)
{
	QVector<iAImageTreeNode const *> likes, hates;
	FindByAttitude(m_treeView->GetTree()->m_root.data(), iAImageTreeNode::Liked, likes);
	FindByAttitude(m_treeView->GetTree()->m_root.data(), iAImageTreeNode::Hated, hates);

	QFile f(fileName);
	if (!f.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(0, "GEMSe", "Couldn't open CSV file for writing attribute range rankings!");
		return;
	}
	QTextStream t(&f);
	t << "Liked";
	for (int i = 0; i < likes.size(); ++i)
	{
		t << "," << likes[i]->GetID();
	}
	t << "\n";
	t << "Disliked";
	for (int i = 0; i < hates.size(); ++i)
	{
		t << "," << hates[i]->GetID();
	}
	t << "\n";
}



void SetAttitude(iAImageTreeNode * node, 
	iAImageTreeNode::Attitude att, int id)
{
	if (node->GetID() == id)
	{
		node->SetAttitude(att);
	}
	for (int i = 0; i<node->GetChildCount(); ++i)
	{
		SetAttitude(node->GetChild(i).data(), att, id);
	}
}


void SetAttitude(iAImageTreeNode * root, QStringList ids, iAImageTreeNode::Attitude att)
{
	for (int i = 1; i < ids.size(); ++i)
	{
		bool ok;
		int id = ids[i].toInt(&ok);
		if (!ok)
		{
			QMessageBox::warning(0, "GEMSe", QString("Invalid ID in rankings file (%1)!").arg(ids[i]));
			return;
		}
		SetAttitude(root, att, id);
	}
}


void dlg_GEMSe::ImportRankings(QString const & fileName)
{
	QFile f(fileName);
	if (!f.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(0, "GEMSe", "Couldn't open CSV file for writing rankings!");
		return;
	}
	QTextStream t(&f);
	QString likes = t.readLine();
	QStringList likeIDs = likes.split(",");
	if (likeIDs.size() == 0 || likeIDs[0] != "Liked")
	{
		QMessageBox::warning(0, "GEMSe", "Invalid rankings file format!");
		return;
	}
	QString hates = t.readLine();
	QStringList hateIDs = hates.split(",");
	if (hateIDs.size() == 0 || hateIDs[0] != "Disliked")
	{
		QMessageBox::warning(0, "GEMSe", "Invalid rankings file format!");
		return;
	}

	SetAttitude(m_treeView->GetTree()->m_root.data(), likeIDs, iAImageTreeNode::Liked);
	SetAttitude(m_treeView->GetTree()->m_root.data(), hateIDs, iAImageTreeNode::Hated);

	UpdateAttributeRangeAttitude();
	m_treeView->UpdateSubtreeHighlight();
	// TODO: update detail view?
}

void dlg_GEMSe::JumpToNode(iAImageTreeNode * node, int stepLimit)
{
	if (!node)
	{
		m_logger->log("JumpToNode: No node selected!");
		return;
	}
	m_treeView->JumpToNode(node, stepLimit);
	//m_exampleView->SetSelectedImage(leaf);
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
	m_previewWidgetPool->SetSliceNumber(sliceNr);
	m_detailView->SetSliceNumber(sliceNr);
}


void dlg_GEMSe::SlicerModeChanged(iASlicerMode mode, int sliceNr)
{
	m_previewWidgetPool->SetSlicerMode(mode, sliceNr, m_cameraWidget->GetCommonCamera());
	m_detailView->SetSlicerMode(mode, sliceNr);
}


void dlg_GEMSe::UpdateViews()
{
	m_cameraWidget->UpdateView();
	m_previewWidgetPool->UpdateViews();
}


void dlg_GEMSe::ShowImage(vtkSmartPointer<vtkImageData> imgData)
{
	if (!m_cameraWidget)
	{
		m_logger->log("ShowImage: Camera Widget not set!");
		return;
	}
	m_cameraWidget->ShowImage(imgData);
}


QSharedPointer<iAImageTreeNode> dlg_GEMSe::GetCurrentCluster()
{
	return m_selectedCluster;
}

void dlg_GEMSe::ChartDblClicked(int chartID)
{
	double min, max;
	GetClusterMinMax(m_selectedCluster.data(), chartID, min, max, m_chartAttributeMapper);
	m_histogramContainer->SetSpanValues(chartID, min, max);
}

void dlg_GEMSe::CalculateRefImgComp(QSharedPointer<iAImageTreeNode> node, LabelImagePointer refImg,
	int labelCount)
{
	if (node->IsLeaf())
	{
		iAImageTreeLeaf * leaf = dynamic_cast<iAImageTreeLeaf*>(node.data());
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(leaf->GetDetailImage().GetPointer());
		double measures[MeasureCount];
		CalculateMeasures(refImg, lblImg, labelCount, measures[0], measures[1], measures[2], measures[3], measures[4]);
		for (int i=0; i<MeasureCount; ++i)
		{
			int chartID = m_MeasureChartIDStart + i;
			int attributeID = m_chartAttributeMapper.GetAttributeID(chartID, leaf->GetDatasetID());
			leaf->SetAttribute(attributeID, measures[i]);
			m_chartAttributes->at(chartID)->AdjustMinMax(measures[i]);
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
		DEBUG_LOG("Reference image comparison calculate: NULL reference image (maybe wrong image type?)!");
		return;
	}
	if (m_chartAttributes->size() == m_MeasureChartIDStart)
	{
		QVector<QSharedPointer<iAAttributeDescriptor> > measures;
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Dice", iAAttributeDescriptor::DerivedOutput, Continuous)));
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Kappa", iAAttributeDescriptor::DerivedOutput, Continuous)));
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Overall Accuracy", iAAttributeDescriptor::DerivedOutput, Continuous)));
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Precision", iAAttributeDescriptor::DerivedOutput, Continuous)));
		measures.push_back(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Recall", iAAttributeDescriptor::DerivedOutput, Continuous)));

		for (QSharedPointer<iAAttributeDescriptor> measure : measures)
		{
			int chartID = m_chartAttributes->size();
			m_chartAttributes->Add(measure);
			// add mappings:
			for (int sampleIdx = 0; sampleIdx < m_samplings.size(); ++sampleIdx)
			{
				QSharedPointer<iAAttributes> attribs = m_samplings[sampleIdx]->GetAttributes();
				int attributeID = attribs->size();
				int datasetID = m_samplings[sampleIdx]->GetID();
				attribs->Add(measure);
				m_chartAttributeMapper.Add(datasetID, attributeID, chartID);
			}
		}
	}
	for (int i= m_MeasureChartIDStart; i<m_MeasureChartIDStart+MeasureCount; ++i)
	{
		m_chartAttributes->at(i)->ResetMinMax();
	}
	CalculateRefImgComp(m_treeView->GetTree()->m_root, refImg, m_treeView->GetTree()->GetLabelCount());
	m_histogramContainer->CreateCharts(m_chartAttributes, m_chartAttributeMapper, m_treeView->GetTree()->m_root.data());
	UpdateClusterChartData();
}


void dlg_GEMSe::SetColorTheme(iAColorTheme const * colorTheme, iALabelInfo const& labelInfo)
{
	m_colorTheme = colorTheme;
	m_previewWidgetPool->SetColorTheme(colorTheme);
	m_detailView->SetLabelInfo(labelInfo);
}

void dlg_GEMSe::ToggleAutoShrink()
{
	m_treeView->SetAutoShrink(!m_treeView->GetAutoShrink());
}

void dlg_GEMSe::SetMagicLensOpacity(double opacity)
{
	m_detailView->SetMagicLensOpacity(opacity);
}


void dlg_GEMSe::SetIconSize(int iconSize)
{
	m_treeView->SetIconSize(iconSize);
}


void dlg_GEMSe::SetRepresentativeType(int type)
{
	m_treeView->SetRepresentativeType(type);
	m_detailView->SetRepresentativeType(type);
}
