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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "dlg_GEMSe.h"

#include "iAAttributes.h"
#include "iACameraWidget.h"
#include "iAChartSpanSlider.h"
#include "iAConsole.h"
#include "iADetailView.h"
#include "iAExampleImageWidget.h"
#include "iAFavoriteWidget.h"
#include "iAImagePreviewWidget.h"
#include "iAImageTreeView.h"
#include "iALogger.h"
#include "iAMathUtility.h"
#include "iAParamHistogramData.h"
#include "iAPreviewWidgetPool.h"
#include "iAAttributeDescriptor.h"
#include "iAQtCaptionWidget.h"
#include "iAGEMSeConstants.h"
#include "iASpectraDistance.h"
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

void dlg_GEMSe::AddDiagramSubWidgetsWithProperStretch()
{
	QHBoxLayout* chartLay = dynamic_cast<QHBoxLayout*>(wdCharts->layout());
	if (findChild<QWidget*>() )
	{
		chartLay->removeWidget(m_chartContainer);
	}
	int paramChartsShownCount = 0,
		derivedChartsShownCount = 0;
	for (AttributeID id = 0; id != m_attributes->GetCount(); ++id )
	{
		if (m_attributes->Get(id)->GetMin() == m_attributes->Get(id)->GetMax())
		{
			//DebugOut() << "Only one value for attribute " << id << ", not showing chart." << std::endl;
			continue;
		}
		if (m_attributes->Get(id)->GetAttribType() == iAAttributeDescriptor::Parameter)
		{
			paramChartsShownCount++;
		}
		else
		{
			derivedChartsShownCount++;
		}
	}
	chartLay->addWidget(m_chartContainer);
	m_chartContainer->addWidget(m_paramChartContainer);
	m_chartContainer->addWidget(m_derivedOutputChartContainer);
	m_chartContainer->setStretchFactor(0, paramChartsShownCount);
	m_chartContainer->setStretchFactor(1, derivedChartsShownCount);
}

void dlg_GEMSe::SetTree(
	QSharedPointer<iAImageTree > imageTree,
	vtkSmartPointer<vtkImageData> originalImage,
	QSharedPointer<iAAttributes> attributes,
	QSharedPointer<iAModalityList> modalities,
	iALabelInfo const & labelInfo)
{
	// reset previous
	if (m_treeView)
	{
		RemoveAllCharts();
		delete m_chartWidget;
		delete m_paramChartContainer;
		delete m_favoriteWidget;
		delete m_detailView;
		delete m_exampleView;
		delete m_cameraWidget;
		delete m_treeView;
		delete m_previewWidgetPool;
	}

	assert(imageTree);
	assert(attributes);
	assert(originalImage);
	if (!imageTree || !attributes || !originalImage)
	{
		return;
	}
	m_attributes = attributes;
	m_refCompMeasureStart = m_attributes->GetCount();
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

	m_detailView = new iADetailView(m_previewWidgetPool->GetWidget(this, true), m_nullImage, modalities, m_attributes, labelInfo,
		m_representativeType);
	m_detailView->SetNode(m_selectedCluster.data());
	m_previewWidgetPool->SetSliceNumber(m_detailView->GetSliceNumber());
	wdImagePreview->layout()->addWidget(m_detailView);

	int extent[6];
	originalImage->GetExtent(extent);
	                                        // height             / width
	double aspectRatio = static_cast<double>(extent[3]-extent[2]) / (extent[1]-extent[0]);

	m_exampleView = new iAExampleImageWidget(wdExamples, aspectRatio, m_previewWidgetPool, m_nullImage);
	wdExamples->layout()->addWidget(m_exampleView);
	m_exampleView->SetSelectedNode(m_selectedCluster);

	m_favoriteWidget = new iAFavoriteWidget(m_previewWidgetPool);
	wdFavorites->layout()->addWidget(m_favoriteWidget);
	wdFavorites->hide();

	m_chartWidget = new QVTKWidget2();
	vtkSmartPointer<vtkContextView> contextView(vtkSmartPointer<vtkContextView>::New());
	contextView->SetRenderWindow(m_chartWidget->GetRenderWindow());
	m_comparisonChart = vtkSmartPointer<vtkChartXY>::New();
	m_comparisonChart->SetSelectionMode(vtkContextScene::SELECTION_NONE);
	contextView->GetScene()->AddItem(m_comparisonChart);
	wdComparisonCharts->layout()->addWidget(m_chartWidget);
	wdComparisonCharts->hide();

	m_paramChartContainer = new QWidget();
	m_paramChartWidget = new QWidget();
	m_paramChartWidget->setLayout(new QHBoxLayout());
	m_paramChartWidget->layout()->setSpacing(ChartSpacing);
	m_paramChartWidget->layout()->setMargin(0);
	SetCaptionedContent(m_paramChartContainer, "Input Parameters", m_paramChartWidget);
	m_chartContainer = new QSplitter();
	m_derivedOutputChartContainer = new QWidget();
	m_derivedOutputChartWidget = new QWidget();
	m_derivedOutputChartWidget->setLayout(new QHBoxLayout());
	m_derivedOutputChartWidget->layout()->setSpacing(ChartSpacing);
	m_derivedOutputChartWidget->layout()->setMargin(0);
	SetCaptionedContent(m_derivedOutputChartContainer, "Derived Output", m_derivedOutputChartWidget);
	dynamic_cast<QHBoxLayout*>(wdCharts->layout())->setSpacing(ChartSpacing);

	connect(m_cameraWidget, SIGNAL(ModeChanged(iASlicerMode, int)), this, SLOT(SlicerModeChanged(iASlicerMode, int)));
	connect(m_treeView, SIGNAL(Clicked(QSharedPointer<iAImageClusterNode>)), this, SLOT(ClusterNodeClicked(QSharedPointer<iAImageClusterNode>)));
	connect(m_treeView, SIGNAL(ImageClicked(QSharedPointer<iAImageClusterNode>)), this, SLOT(ClusterNodeImageClicked(QSharedPointer<iAImageClusterNode>)));
	connect(m_treeView, SIGNAL(Expanded(QSharedPointer<iAImageClusterNode>)), this, SLOT(SelectCluster(QSharedPointer<iAImageClusterNode>)));
	connect(m_treeView, SIGNAL(JumpedTo(QSharedPointer<iAImageClusterNode>)), this, SLOT(SelectCluster(QSharedPointer<iAImageClusterNode>)));
	connect(m_treeView, SIGNAL(SelectionChanged()), this, SLOT(UpdateClusterChartData()));
	connect(m_exampleView, SIGNAL(Selected(iAImageClusterLeaf *)), this, SLOT(ClusterLeafSelected(iAImageClusterLeaf *)));
	connect(m_detailView, SIGNAL(Like()), this, SLOT(ToggleLike()));
	connect(m_detailView, SIGNAL(Hate()), this, SLOT(ToggleHate()));
	connect(m_detailView, SIGNAL(GoToCluster()), this, SLOT(GoToCluster()));
	connect(m_cameraWidget, SIGNAL(SliceChanged(int)), this, SLOT(SliceNumberChanged(int)));
	connect(m_favoriteWidget, SIGNAL(Clicked(iAImageClusterNode *)), this, SLOT(FavoriteClicked(iAImageClusterNode *)));

	// view updates:
	connect(m_detailView,     SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );
	connect(m_cameraWidget,   SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );
	connect(m_treeView,       SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );
	connect(m_exampleView,    SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );
	connect(m_favoriteWidget, SIGNAL(ViewUpdated()), this, SLOT(UpdateViews()) );

	RecreateCharts();
}


void ClearFilteredRep(QSharedPointer<iAImageClusterNode> node)
{
	if (!node->IsLeaf())
	{
		((iAImageClusterInternal*)node.data())->DiscardFilterData();
		for (int i = 0; i < node->GetChildCount(); ++i)
		{
			ClearFilteredRep(node->GetChild(i));
		}
	}}


void dlg_GEMSe::UpdateComparisonChart()
{
	iAAttributeFilter emptyFilter;
	m_allTable = GetComparisonTable(m_treeView->GetTree()->m_root.data(), emptyFilter); // TODO do we need m_comparisonTable as member?
	if (!m_allTable)
	{
		wdComparisonCharts->hide();
		return;
	}
	m_comparisonChart->ClearPlots();
	m_singlePlot = 0;
	m_clusterPlot = 0;
	auto xAxis = m_comparisonChart->GetAxis(vtkAxis::BOTTOM);
	auto yAxis = m_comparisonChart->GetAxis(vtkAxis::LEFT);
	int idx1 = m_selectedForComparison[0];
	int idx2 = m_selectedForComparison[0];
	xAxis->SetTitle(m_attributes->Get(idx1)->GetName().toStdString());
	xAxis->SetLogScale(m_attributes->Get(idx1)->IsLogScale() );
	yAxis->SetTitle(m_attributes->Get(idx2)->GetName().toStdString());
	yAxis->SetLogScale(m_attributes->Get(idx2)->IsLogScale() );

	vtkPlot* plot = m_comparisonChart->AddPlot(vtkChart::POINTS);
	plot->SetColor(
		static_cast<unsigned char>(DefaultColors::AllDataChartColor.red()),
		static_cast<unsigned char>(DefaultColors::AllDataChartColor.green()),
		static_cast<unsigned char>(DefaultColors::AllDataChartColor.blue()),
		static_cast<unsigned char>(DefaultColors::AllDataChartColor.alpha())
	);
	plot->SetWidth(1.0);
	plot->SetInputData(m_allTable, 0, 1);
	UpdateComparisonChartClusterPlot();
	UpdateComparisonChartLeafPlot();
	if (!m_attributeFilter.MatchesAll())
	{
		UpdateComparisonChart(m_allFilteredTable, m_allFilteredPlot, m_treeView->GetTree()->m_root.data(), DefaultColors::FilteredChartColor, m_attributeFilter);
		ClearFilteredRep(m_treeView->GetTree()->m_root);
	}
}

void dlg_GEMSe::UpdateComparisonChart(vtkSmartPointer<vtkTable> & table, vtkSmartPointer<vtkPlot> & plot, iAImageClusterNode const * node,
	QColor const & color, iAAttributeFilter const & filter)
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

void dlg_GEMSe::UpdateComparisonChartLeafPlot()
{
	iAAttributeFilter emptyFilter;
	UpdateComparisonChart(m_singleTable, m_singlePlot, m_selectedLeaf, DefaultColors::ImageChartColor, emptyFilter);
}

void dlg_GEMSe::UpdateComparisonChartClusterPlot()
{
	iAAttributeFilter emptyFilter;
	UpdateComparisonChart(m_clusterTable, m_clusterPlot, m_selectedCluster.data(), DefaultColors::ClusterChartColor[0], emptyFilter);	
	if (!m_attributeFilter.MatchesAll())
	{
		UpdateComparisonChart(m_clusterFilteredTable, m_clusterFilteredPlot, m_selectedCluster.data(), DefaultColors::FilteredClusterChartColor, m_attributeFilter);
	}
}

void dlg_GEMSe::RemoveAllCharts()
{
	for (AttributeID id = 0; id != m_attributes->GetCount(); ++id)
	{
		if (m_charts.contains(id))
		{
			delete m_charts[id];
			m_charts.remove(id);
		}
	}
	m_charts.clear();
}

void dlg_GEMSe::RecreateCharts()
{
	if (!m_attributes)
	{
		return;
	}
	AddDiagramSubWidgetsWithProperStretch();
	RemoveAllCharts();
	for (AttributeID id = 0; id != m_attributes->GetCount(); ++id )
	{
		if (m_attributes->Get(id)->GetMin() == m_attributes->Get(id)->GetMax())
		{
			//DebugOut() << "Only one value for attribute " << id << ", not showing chart." << std::endl;
			continue;
		}

		QSharedPointer<iAParamHistogramData> data = iAParamHistogramData::Create(m_selectedCluster.data(), id,
			m_attributes->Get(id)->GetValueType(),
			m_attributes->Get(id)->GetMin(),
			m_attributes->Get(id)->GetMax(),
			m_attributes->Get(id)->IsLogScale());
		if (!data)
		{
			DEBUG_LOG(QString("ERROR: Creating chart data for attribute %1 failed!\n").arg(id));
			continue;
		}

		m_charts.insert(id, new iAChartSpanSlider(m_attributes->Get(id)->GetName(), id, data,
			m_attributes->Get(id)->GetNameMapper() ));
		
		connect(m_charts[id], SIGNAL(Toggled(bool)), this, SLOT(ChartSelected(bool)));
		connect(m_charts[id], SIGNAL(FilterChanged(double, double)), this, SLOT(FilterChanged(double, double)));
		connect(m_charts[id], SIGNAL(ChartDblClicked()), this, SLOT(ChartDblClicked()));
	
		if (m_attributes->Get(id)->GetAttribType() == iAAttributeDescriptor::Parameter)
		{
			m_paramChartWidget->layout()->addWidget(m_charts[id]);
		}
		else
		{
			m_derivedOutputChartWidget->layout()->addWidget(m_charts[id]);
		}
		m_charts[id]->update();
	}
	UpdateClusterChartData();
}

void AddTableValues(vtkSmartPointer<vtkTable> table, iAImageClusterNode const * node, AttributeID p1, AttributeID p2, int & rowNr, iAAttributeFilter const & attribFilter)
{
	if (node->IsLeaf())
	{
		iAImageClusterLeaf const * leaf = dynamic_cast<iAImageClusterLeaf const *>(node);
		if (attribFilter.Matches(leaf))
		{
			table->SetValue(rowNr, 0, node->GetAttribute(p1));
			table->SetValue(rowNr, 1, node->GetAttribute(p2));
			rowNr++;
		}
	}
	else
	{
		for (int i=0; i<node->GetChildCount(); ++i)
		{
			AddTableValues(table, node->GetChild(i).data(), p1, p2, rowNr, attribFilter);
		}
	}
}

vtkSmartPointer<vtkTable> dlg_GEMSe::GetComparisonTable(iAImageClusterNode const * node, iAAttributeFilter const & attribFilter)
{
	if (!node || m_selectedForComparison.size() < 2)
	{
		return vtkSmartPointer<vtkTable>();
	}
	vtkSmartPointer<vtkTable> comparisonTable = vtkSmartPointer<vtkTable>::New();

	int numberOfSamples = node->GetClusterSize();
	vtkSmartPointer<vtkFloatArray> arrX(vtkSmartPointer<vtkFloatArray>::New());
	vtkSmartPointer<vtkFloatArray> arrY(vtkSmartPointer<vtkFloatArray>::New());
	arrX->SetName(m_attributes->Get(m_selectedForComparison[0])->GetName().toStdString().c_str());  // TODO: get
	arrY->SetName(m_attributes->Get(m_selectedForComparison[1])->GetName().toStdString().c_str());
	comparisonTable->AddColumn(arrX);
	comparisonTable->AddColumn(arrY);
	// we may call SetNumberOfRows only after we've added all columns, otherwise there is an error
	comparisonTable->SetNumberOfRows(numberOfSamples);

	int rowNr = 0;
	AddTableValues(comparisonTable, node, m_selectedForComparison[0], m_selectedForComparison[1], rowNr, attribFilter); 

	// reduce number of rows to actual number:
	comparisonTable->SetNumberOfRows(rowNr);
	return comparisonTable;
}

void dlg_GEMSe::ClusterNodeClicked(QSharedPointer<iAImageClusterNode> node)
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


void dlg_GEMSe::ClusterNodeImageClicked(QSharedPointer<iAImageClusterNode> node)
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

void dlg_GEMSe::SelectCluster(QSharedPointer<iAImageClusterNode> node)
{
	m_selectedCluster = node;
	m_selectedLeaf = 0;
	bool clear = !QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
	m_treeView->AddSelectedNode(node, clear);
	m_exampleView->SetSelectedNode(node);
	UpdateClusterFilteredChartData();
	UpdateComparisonChartClusterPlot();
	UpdateComparisonChartLeafPlot();
	if (node->IsLeaf())
	{
		iAImageClusterLeaf * leaf = dynamic_cast<iAImageClusterLeaf*>(node.data());
		ClusterLeafSelected(leaf);
	}
	else
	{
		m_detailView->SetNode(node.data());
	}
}


void dlg_GEMSe::ClusterLeafSelected(iAImageClusterLeaf * node)
{
	m_selectedLeaf = node;
	m_detailView->SetNode(node);

	for (AttributeID id=0; id<m_attributes->GetCount(); ++id)
	{
		if (!m_charts.contains(id) || !m_charts[id])
		{
			continue;
		}
		double value = node->GetAttribute(id);
		if (m_attributes->Get(id)->GetValueType() == Discrete || m_attributes->Get(id)->GetValueType() == Categorical)
		{
			value += 0.5;
		}
		m_charts[id]->SetMarker(value);
	}
	UpdateComparisonChartLeafPlot();
}

void dlg_GEMSe::StoreClustering(QString const & fileName)
{
	m_treeView->GetTree()->Store(fileName);
}


void dlg_GEMSe::ChartSelected(bool selected)
{
	iAChartSpanSlider* chart = dynamic_cast<iAChartSpanSlider*>(sender());
	
	AttributeID id = m_charts.key(chart);
	if (selected)
	{
		m_selectedForComparison.push_back(id);
	}
	else
	{
		int idx = m_selectedForComparison.indexOf(id);
		assert (idx != -1);
		if (idx != -1)
		{
			m_selectedForComparison.remove(idx);
		}
	}
	UpdateComparisonChart();
	// ...
}

void dlg_GEMSe::UpdateClusterChartData()
{
	QVector<QSharedPointer<iAImageClusterNode> > const selection =  m_treeView->CurrentSelection();
	for (AttributeID id=0; id<m_attributes->GetCount(); ++id )
	{
		if (!m_charts.contains(id) || !m_charts[id])
		{
			continue;
		}
		m_charts[id]->ClearClusterData();
		foreach(QSharedPointer<iAImageClusterNode> const node, selection)
		{
			m_charts[id]->AddClusterData(iAParamHistogramData::Create(node.data(), id,
				m_attributes->Get(id)->GetValueType(),
				m_attributes->Get(id)->GetMin(),
				m_attributes->Get(id)->GetMax(),
				m_attributes->Get(id)->IsLogScale()));
		}
		m_charts[id]->UpdateChart();
	}
}

void dlg_GEMSe::UpdateClusterFilteredChartData()
{
	for (AttributeID id=0; id<m_attributes->GetCount(); ++id)
	{
		if (!m_charts.contains(id) || !m_charts[id])
		{
			continue;
		}
		assert (m_charts[id]);
		if (m_attributeFilter.MatchesAll())
		{
			m_charts[id]->RemoveFilterData();
		}
		else
		{
			m_charts[id]->SetFilteredClusterData(iAParamHistogramData::Create(m_selectedCluster.data(), id,
				m_attributes->Get(id)->GetValueType(),
				m_attributes->Get(id)->GetMin(),
				m_attributes->Get(id)->GetMax(),
				m_attributes->Get(id)->IsLogScale(), m_attributeFilter));
		}
	}
}

void dlg_GEMSe::UpdateFilteredChartData()
{
	for (AttributeID id=0; id<m_attributes->GetCount(); ++id)
	{
		if (!m_charts.contains(id) || !m_charts[id])
		{
			continue;
		}
		assert (m_charts[id]);
		m_charts[id]->SetFilteredData(iAParamHistogramData::Create(m_treeView->GetTree()->m_root.data(), id,
			m_attributes->Get(id)->GetValueType(),
			m_attributes->Get(id)->GetMin(),
			m_attributes->Get(id)->GetMax(),
			m_attributes->Get(id)->IsLogScale(), m_attributeFilter));
	}
}


void dlg_GEMSe::UpdateFilteredData()
{
	m_treeView->GetTree()->m_root->UpdateFilter(m_attributeFilter);
	m_treeView->FilterUpdated();
	m_exampleView->FilterUpdated();

	if (m_detailView->IsShowingCluster())
	{
		m_detailView->SetNode(m_selectedCluster.data());
	}
	UpdateFilteredChartData();
	UpdateClusterFilteredChartData();
	if (!m_attributeFilter.MatchesAll())
	{
		UpdateComparisonChart(m_allFilteredTable, m_allFilteredPlot, m_treeView->GetTree()->m_root.data(), DefaultColors::FilteredChartColor, m_attributeFilter);
	}
	UpdateComparisonChartClusterPlot();
}

void dlg_GEMSe::FilterChanged(double min, double max)
{
	iAChartSpanSlider* slider = dynamic_cast<iAChartSpanSlider*>(sender());
	assert(slider);
	if (!slider)
	{
		DEBUG_LOG("FilterChanged called from non-slider widget...\n");
		return;
	}
	AttributeID attribID = slider->GetAttribID();
	if (m_attributes->Get(attribID)->CoversWholeRange(min, max))
	{
		m_attributeFilter.RemoveFilter(attribID);
	}
	else
	{
		m_attributeFilter.AddFilter(attribID, min, max);
	}
	
	UpdateFilteredData();
}

void dlg_GEMSe::ResetFilters()
{
	if (!m_attributes)
	{
		return;
	}
	m_attributeFilter.Reset();
	for (AttributeID id = 0; id != m_attributes->GetCount(); ++id )
	{
		if (!m_charts.contains(id) || !m_charts[id])
		{
			continue;
		}
		QSignalBlocker blocker(m_charts[id]);
		m_charts[id]->ResetSpan();
	}
	UpdateFilteredData();
}


void dlg_GEMSe::ToggleHate()
{
	iAImageClusterNode* node = (m_selectedLeaf) ? m_selectedLeaf : m_selectedCluster.data();
	if (!node)
	{
		m_logger->log("ToggleHate No node selected!");
		return;
	}
	bool isHated = m_favoriteWidget->ToggleHate(node);
	m_detailView->UpdateLikeHate(false, node->GetAttitude() == iAImageClusterNode::Hated);
	m_treeView->UpdateAutoShrink(node, isHated);
	m_treeView->UpdateSubtreeHighlight();

	wdFavorites->setVisible(m_favoriteWidget->HasAnyFavorite());

	UpdateAttributeRangeAttitude();
}


void dlg_GEMSe::ToggleLike()
{
	iAImageClusterNode* node = (m_selectedLeaf) ? m_selectedLeaf : m_selectedCluster.data();
	if (!node)
	{
		m_logger->log("ToggleHate No node selected!");
		return;
	}
	m_favoriteWidget->ToggleLike(node);
	m_detailView->UpdateLikeHate(node->GetAttitude() == iAImageClusterNode::Liked, false);
	m_treeView->UpdateSubtreeHighlight();

	wdFavorites->setVisible(m_favoriteWidget->HasAnyFavorite());

	UpdateAttributeRangeAttitude();
}

struct AttributeHistogram
{
	int * data;
	AttributeHistogram(int numBins) :
		data(new int[numBins])
	{
		std::fill(data, data+numBins, 0);
	}
	~AttributeHistogram()
	{
		delete[] data;
	}
	AttributeHistogram(const AttributeHistogram & other) = delete;
	AttributeHistogram & operator=(const AttributeHistogram & other) = delete;
};

void AddClusterData(AttributeHistogram & hist, iAImageClusterNode const * node, AttributeID id, iAChartSpanSlider* chart, int numBin)
{
	if (node->IsLeaf())
	{
		int bin = clamp(0, numBin-1, static_cast<int>(chart->mapValueToBin(node->GetAttribute(id))));
		hist.data[bin]++;
	}
	else
	{
		for (int i=0; i<node->GetChildCount(); ++i)
		{
			AddClusterData(hist, node->GetChild(i).data(), id, chart, numBin);
		}
	}
}

void GetHistData(AttributeHistogram & hist, AttributeID id, iAChartSpanSlider* chart, QVector<iAImageClusterNode const *> const & nodes, int numBin)
{
	for (int l = 0; l < nodes.size(); ++l)
	{
		iAImageClusterNode const * node = nodes[l];
		AddClusterData(hist, node, id, chart, numBin);
	}
}

void FindByAttitude(iAImageClusterNode const * node, iAImageClusterNode::Attitude att, QVector<iAImageClusterNode const *> & nodeList)
{
	if (node->GetAttitude() == att)
	{
		nodeList.push_back(node);
	}
	for (int i=0; i<node->GetChildCount(); ++i)
	{
		FindByAttitude(node->GetChild(i).data(), att, nodeList);
	}
}

void dlg_GEMSe::UpdateAttributeRangeAttitude()
{
	QVector<iAImageClusterNode const *> likes, hates;
	FindByAttitude(m_treeView->GetTree()->m_root.data(), iAImageClusterNode::Liked, likes);
	FindByAttitude(m_treeView->GetTree()->m_root.data(), iAImageClusterNode::Hated, hates);

	m_attitudes.clear();
	for (AttributeID id = 0; id != m_attributes->GetCount(); ++id)
	{
		m_attitudes.push_back(QVector<float>());
		if (!m_charts.contains(id) || !m_charts[id])
		{
			continue;
		}
		int numBin = m_charts[id]->GetNumBin();
		AttributeHistogram likeHist(numBin);
		GetHistData(likeHist, id, m_charts[id], likes, numBin);
		AttributeHistogram hateHist(numBin);
		GetHistData(hateHist, id, m_charts[id], hates, numBin);

		for (int b = 0; b < numBin; ++b)
		{
			QColor color(0, 0, 0, 0);
			double attitude = (likeHist.data[b] + hateHist.data[b]) == 0 ? 0 :
				(likeHist.data[b] - hateHist.data[b])
				/ static_cast<double>(likeHist.data[b] + hateHist.data[b]);
			if (attitude > 0) // user likes this region
			{
				color.setGreen(attitude * 255);
				color.setAlpha(attitude * 100);
			}
			else
			{
				color.setRed(-attitude * 255);
				color.setAlpha(-attitude * 100);
			}
			m_attitudes[id].push_back(attitude);
			m_charts[id]->SetBinColor(b, color);
			m_charts[id]->UpdateChart();
		}
	}
}


void dlg_GEMSe::ExportAttributeRangeRanking(QString const &fileName)
{
	QFile f(fileName);
	if (!f.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(0, "GEMSe", "Couldn't open CSV file for writing attribute range rankings!");
		return;
	}
	QTextStream t(&f);
	for (int i=0; i<m_attitudes.size(); ++i)
	{
		t << m_attributes->Get(i)->GetName();
		if (!m_charts[i])
		{
			t << "\n";
			continue;
		}
		size_t numBin = m_charts[i]->GetNumBin();
		double min = m_attributes->Get(i)->GetMin();
		double max = m_attributes->Get(i)->GetMax();
		t << "," << min << "," << max << "," << numBin;
		for (int b = 0; b < m_attitudes[i].size(); ++b)
		{
			t << "," << m_attitudes[i][b];
		}
		t << "\n";
	}
	
}

void dlg_GEMSe::ExportRankings(QString const & fileName)
{
	QVector<iAImageClusterNode const *> likes, hates;
	FindByAttitude(m_treeView->GetTree()->m_root.data(), iAImageClusterNode::Liked, likes);
	FindByAttitude(m_treeView->GetTree()->m_root.data(), iAImageClusterNode::Hated, hates);

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



void SetAttitude(iAImageClusterNode * node, 
	iAImageClusterNode::Attitude att, int id)
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


void SetAttitude(iAImageClusterNode * root, QStringList ids, iAImageClusterNode::Attitude att)
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

	SetAttitude(m_treeView->GetTree()->m_root.data(), likeIDs, iAImageClusterNode::Liked);
	SetAttitude(m_treeView->GetTree()->m_root.data(), hateIDs, iAImageClusterNode::Hated);

	UpdateAttributeRangeAttitude();
	m_treeView->UpdateSubtreeHighlight();
	// TODO: update detail view?
}

void dlg_GEMSe::JumpToNode(iAImageClusterNode * node, int stepLimit)
{
	if (!node)
	{
		m_logger->log("JumpToNode: No node selected!");
		return;
	}
	m_treeView->JumpToNode(node, stepLimit);
	//m_exampleView->SetSelectedImage(leaf);
}

void dlg_GEMSe::FavoriteClicked(iAImageClusterNode * node)
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


QSharedPointer<iAImageClusterNode> dlg_GEMSe::GetCurrentCluster()
{
	return m_selectedCluster;
}

void dlg_GEMSe::ChartDblClicked()
{
	iAChartSpanSlider* slider = dynamic_cast<iAChartSpanSlider*>(sender());
	assert(slider);
	if (!slider)
	{
		DEBUG_LOG("FilterChanged called from non-slider widget...\n");
		return;
	}
	AttributeID attribID = slider->GetAttribID();
	double min, max;
	GetClusterMinMax(m_selectedCluster.data(), attribID, min, max);
	slider->SetSpanValues(min, max);
}


#include <itkLabelOverlapMeasuresImageFilter.h>

class Matrix
{
private:
	int * data;
	int rowCount, colCount;
public:
	Matrix (int rowCount, int colCount):
		rowCount(rowCount), colCount(colCount)
	{
		data = new int[rowCount * colCount];
		for (int i=0; i<rowCount * colCount; ++i)
		{
			data[i] = 0;
		}
	}
	~Matrix()
	{
		delete [] data;
	}
	int & get(int r, int c)
	{
		return data[r*colCount + c];
	}
	void inc(int r, int c)
	{
		get(r,c)++;
	}
	int RowCount() const { return rowCount; }
	int ColCount() const { return colCount; }
};

int * rowTotals(Matrix & m)
{
	int * result = new int[m.RowCount()];
	for (int r=0; r<m.RowCount(); ++r)
	{
		result[r] = 0;
		for (int c=0; c<m.ColCount(); ++c)
		{
			result[r] += m.get(r, c);
		}
	}
	return result;
}
int * colTotals(Matrix & m)
{
	int * result = new int[m.ColCount()];
	for (int c=0; c<m.ColCount(); ++c)
	{
		result[c] = 0;
		for (int r=0; r<m.RowCount(); ++r)
		{
			result[c] += m.get(r, c);
		}
	}
	return result;
}

void CalculateMeasures(LabelImagePointer refImg, LabelImagePointer curImg, int labelCount, double & kappa, double & oa, double &precision, double &recall)
{
	                   // row      column
	Matrix errorMatrix(labelCount, labelCount);
	LabelImageType::RegionType reg = refImg->GetLargestPossibleRegion();
	LabelImageType::SizeType size = reg.GetSize();
	LabelImageType::IndexType idx;
	for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
	{	
		for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
		{		
			for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
			{
				int refValue = refImg->GetPixel(idx);
				int curValue = curImg->GetPixel(idx);
				errorMatrix.inc(curValue, refValue);
			}
		}
	}
	int * actTot = rowTotals(errorMatrix);
	int * refTot = colTotals(errorMatrix);

	int diagSum = 0;
	int totalSum = 0;

	precision = 0;
	recall    = 0;
	
	double chanceAgreement = 0;

	for (int i=0; i<labelCount; ++i)
	{
		diagSum += errorMatrix.get(i, i);
		totalSum += actTot[i];
		precision += (actTot[i] == 0) ? 0 : errorMatrix.get(i, i) / static_cast<double>(actTot[i]); // could be equivalent to oa, have to check!
		recall += (refTot[i] == 0) ? 0 : errorMatrix.get(i, i) / static_cast<double>(refTot[i]);

		chanceAgreement = actTot[i] * refTot[i];
	}
	chanceAgreement /= static_cast<double>(pow(totalSum, 2));


	oa = diagSum / static_cast<double>(totalSum);

	kappa =  (oa - chanceAgreement) / (1 - chanceAgreement);

	delete [] actTot;
	delete [] refTot;
}


void dlg_GEMSe::CalculateRefImgComp(QSharedPointer<iAImageClusterNode> node, LabelImagePointer refImg,
	int labelCount)
{
	if (node->IsLeaf())
	{
		iAImageClusterLeaf * leaf = dynamic_cast<iAImageClusterLeaf*>(node.data());
		typedef itk::LabelOverlapMeasuresImageFilter<LabelImageType > FilterType;
		FilterType::Pointer filter = FilterType::New();
		filter->SetSourceImage(refImg);
		LabelImageType* lblImg = dynamic_cast<LabelImageType*>(leaf->GetDetailImage().GetPointer());
		filter->SetTargetImage(lblImg);
		filter->Update();
		double measures[MeasureCount];
		measures[0] = filter->GetMeanOverlap();
		
		CalculateMeasures(refImg, lblImg, labelCount, measures[1], measures[2], measures[3], measures[4]);
		for (int i=0; i<MeasureCount; ++i)
		{
			leaf->SetAttribute(m_refCompMeasureStart + i, measures[i]);
			m_attributes->Get(m_refCompMeasureStart + i)->AdjustMinMax(measures[i]);
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
		DEBUG_LOG("Reference image comparison calculate: NULL reference image (maybe wrong image type?)!\n");
		return;
	}
	if (m_attributes->GetCount() == m_refCompMeasureStart)
	{
		m_attributes->Add(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Dice", iAAttributeDescriptor::DerivedOutput, Continuous)));
		m_attributes->Add(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Kappa", iAAttributeDescriptor::DerivedOutput, Continuous)));
		m_attributes->Add(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Overall Accuracy", iAAttributeDescriptor::DerivedOutput, Continuous)));
		m_attributes->Add(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Precision", iAAttributeDescriptor::DerivedOutput, Continuous)));
		m_attributes->Add(QSharedPointer<iAAttributeDescriptor>(new iAAttributeDescriptor(
			"Recall", iAAttributeDescriptor::DerivedOutput, Continuous)));
	}
	for (int i= m_refCompMeasureStart; i<m_refCompMeasureStart+MeasureCount; ++i)
	{
		m_attributes->Get(i)->ResetMinMax();
	}
	CalculateRefImgComp(m_treeView->GetTree()->m_root, refImg, m_treeView->GetTree()->GetLabelCount());
	RecreateCharts();
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
