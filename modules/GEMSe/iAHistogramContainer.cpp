// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAHistogramContainer.h"

#include "iAAttitudes.h"
#include "iAAttributes.h"
#include "iAChartAttributeMapper.h"
#include "iAChartFilter.h"
#include "iAClusterAttribChart.h"
#include "iAImageTree.h"
#include "iAParamHistogramData.h"
#include "iAQtCaptionWidget.h"

#include <iAAttributeDescriptor.h>
#include <iALog.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QTextStream>

iAHistogramContainer::iAHistogramContainer(
	std::shared_ptr<iAAttributes> chartAttributes,
	iAChartAttributeMapper const & chartAttributeMapper,
	iAImageTreeNode const * root,
	QStringList const & pipelineNames):
	m_paramChartWidget(new QWidget()),
	m_derivedOutputChartWidget(new QWidget()),
	m_paramChartContainer(new QWidget()),
	m_derivedOutputChartContainer(new QWidget()),
	m_paramChartLayout(nullptr),
	m_chartContainer(new QSplitter()),
	m_chartAttributes(chartAttributes),
	m_chartAttributeMapper(chartAttributeMapper),
	m_root(root),
	m_pipelineNames(pipelineNames)
{
	CreateGridLayout();
	SetCaptionedContent(m_paramChartContainer, "Input Parameters", m_paramChartWidget);
	m_derivedOutputChartWidget->setLayout(new QHBoxLayout());
	m_derivedOutputChartWidget->layout()->setSpacing(ChartSpacing);
	m_derivedOutputChartWidget->layout()->setContentsMargins(0, 0, 0, 0);
	SetCaptionedContent(m_derivedOutputChartContainer, "Derived Output", m_derivedOutputChartWidget);
}

void iAHistogramContainer::CreateGridLayout()
{
	delete m_paramChartLayout;
	m_paramChartLayout = new QGridLayout();
	m_paramChartLayout->setSpacing(ChartSpacing);
	m_paramChartLayout->setContentsMargins(0, 0, 0, 0);
	m_paramChartWidget->setLayout(m_paramChartLayout);
}


void iAHistogramContainer::CreateCharts()
{
	RemoveAllCharts();
	CreateGridLayout();

	addWidget(m_paramChartContainer);
	addWidget(m_derivedOutputChartContainer);

	int curMinDatasetID = 0;
	int paramChartRow = 0;
	int paramChartCol = 1;
	QLabel* lbPipelineName = new QLabel(m_pipelineNames[paramChartRow]);
	m_labels.push_back(lbPipelineName);
	m_paramChartLayout->addWidget(lbPipelineName, paramChartRow, 0);
	int paramChartMaxCols = 0;
	int derivedOutMaxCols = 0;
	double maxValue = -1;
	for (int chartID = 0; chartID != m_chartAttributes->size(); ++chartID)
	{
		auto attrib = m_chartAttributes->at(chartID);
		if (attrib->min() == attrib->max() || m_disabledCharts.contains(chartID))
		{
			continue;
		}
		// maximum number of bins:
		//		- square root of number of values (https://en.wikipedia.org/wiki/Histogram#Number_of_bins_and_width)
		//      - adapting to width of histogram?
		//      - if discrete or categorical values: limit by range
		auto maxBin = std::min(static_cast<size_t>(std::sqrt(m_root->GetClusterSize())), HistogramBinCount);
		auto numBin = (attrib->min() == attrib->max()) ? 1 :
			(attrib->valueType() == iAValueType::Discrete || attrib->valueType() == iAValueType::Categorical) ?
			std::min(static_cast<size_t>(attrib->max() - attrib->min() + 1), maxBin) :
			maxBin;
		auto paramData = iAParamHistogramData::create(
			m_root,
			chartID,
			attrib->valueType(),
			attrib->min(),
			attrib->max(),
			attrib->isLogScale(),
			m_chartAttributeMapper,
			numBin);
		if (!paramData)
		{
			LOG(lvlError, QString("Creating chart #%1 data for attribute %2 failed!").arg(chartID)
				.arg(attrib->name()));
			continue;
		}
		if (attrib->attribType() == iAAttributeDescriptor::Parameter)
		{
			maxValue = std::max(paramData->yBounds()[1], maxValue);
		}
		m_charts.insert(chartID, new iAClusterAttribChart(attrib->name(), chartID, paramData,
			attrib->nameMapper()));

		connect(m_charts[chartID], &iAClusterAttribChart::Toggled, this, &iAHistogramContainer::ChartSelected);
		connect(m_charts[chartID], &iAClusterAttribChart::FilterChanged, this, QOverload<double,double>::of(&iAHistogramContainer::FilterChanged));
		connect(m_charts[chartID], &iAClusterAttribChart::ChartDblClicked, this, QOverload<>::of(&iAHistogramContainer::ChartDblClicked));

		if (attrib->attribType() == iAAttributeDescriptor::Parameter)
		{
			QList<int> datasetIDs = m_chartAttributeMapper.GetDatasetIDs(chartID);
			if (!datasetIDs.contains(curMinDatasetID))
			{
				// alternative to GridLayout: Use combination of VBox and HBox layout?
				curMinDatasetID = datasetIDs[0];
				paramChartCol = 1;
				paramChartRow++;
				QLabel* label = new QLabel(m_pipelineNames[paramChartRow]);
				m_labels.push_back(label);
				m_paramChartLayout->addWidget(label, paramChartRow, 0);
			}
			paramChartMaxCols = std::max(paramChartCol, paramChartMaxCols);
			m_paramChartLayout->addWidget(m_charts[chartID], paramChartRow, paramChartCol);
			m_paramChartLayout->setColumnStretch(paramChartCol, 1);
			paramChartCol++;
		}
		else
		{
			m_derivedOutputChartWidget->layout()->addWidget(m_charts[chartID]);
			derivedOutMaxCols++;
		}
		m_charts[chartID]->update();
	}
	for (int i = 0; i < m_chartAttributes->size(); ++i)
	{
		if (m_charts[i] &&
			m_chartAttributes->at(i)->attribType() == iAAttributeDescriptor::Parameter)
		{
			m_charts[i]->SetMaxYAxisValue(maxValue);
		}
	}
	setStretchFactor(0, paramChartMaxCols);
	setStretchFactor(1, derivedOutMaxCols);
}


void iAHistogramContainer::UpdateClusterChartData(QVector<std::shared_ptr<iAImageTreeNode> > const & selection)
{
	for (int chartID = 0; chartID < m_chartAttributes->size(); ++chartID)
	{
		if (!ChartExists(chartID))
		{
			continue;
		}
		m_charts[chartID]->ClearClusterData();
		for (auto const & node: selection)
		{
			auto attrib = m_chartAttributes->at(chartID);
			m_charts[chartID]->AddClusterData(iAParamHistogramData::create(
				node.get(), chartID,
				attrib->valueType(),
				attrib->min(),
				attrib->max(),
				attrib->isLogScale(),
				m_chartAttributeMapper,
				m_charts[chartID]->GetNumBin()));
		}
		m_charts[chartID]->UpdateChart();
	}
}


void iAHistogramContainer::UpdateClusterFilteredChartData(
	iAImageTreeNode const * selectedNode,
	iAChartFilter const & chartFilter)
{
	for (int chartID = 0; chartID < m_chartAttributes->size(); ++chartID)
	{
		if (!ChartExists(chartID))
		{
			continue;
		}
		assert(m_charts[chartID]);
		if (chartFilter.MatchesAll())
		{
			m_charts[chartID]->RemoveFilterData();
		}
		else
		{
			auto attrib = m_chartAttributes->at(chartID);
			m_charts[chartID]->SetFilteredClusterData(iAParamHistogramData::create(
				selectedNode, chartID,
				attrib->valueType(),
				attrib->min(),
				attrib->max(),
				attrib->isLogScale(),
				m_chartAttributeMapper,
				chartFilter,
				m_charts[chartID]->GetNumBin()));
		}
	}
}


void iAHistogramContainer::UpdateFilteredChartData(iAChartFilter const & chartFilter)
{
	for (int chartID = 0; chartID < m_chartAttributes->size(); ++chartID)
	{
		if (!ChartExists(chartID))
		{
			continue;
		}
		assert(m_charts[chartID]);
		auto attrib = m_chartAttributes->at(chartID);
		m_charts[chartID]->SetFilteredData(iAParamHistogramData::create(
			m_root, chartID,
			attrib->valueType(),
			attrib->min(),
			attrib->max(),
			attrib->isLogScale(),
			m_chartAttributeMapper,
			chartFilter,
			m_charts[chartID]->GetNumBin()));
	}
}


bool iAHistogramContainer::ChartExists(int chartID) const
{
	return m_charts.contains(chartID) && m_charts[chartID];
}


void iAHistogramContainer::RemoveAllCharts()
{
	for (int chartID = 0; chartID != m_chartAttributes->size(); ++chartID)
	{
		if (ChartExists(chartID))
		{
			delete m_charts[chartID];
			m_charts.remove(chartID);
		}
	}
	m_charts.clear();
	for (QLabel* l : m_labels)
	{
		delete l;
	}
	m_labels.clear();
}


void iAHistogramContainer::ChartSelected(bool selected)
{
	iAClusterAttribChart* chart = dynamic_cast<iAClusterAttribChart*>(sender());

	int id = m_charts.key(chart);
	if (selected)
	{
		m_selected.push_back(id);
	}
	else
	{
		auto idx = m_selected.indexOf(id);
		assert(idx != -1);
		if (idx != -1)
		{
			m_selected.remove(idx);
		}
	}
	emit ChartSelectionUpdated();
}


void iAHistogramContainer::ResetFilters()
{

	for (int chartID = 0; chartID != m_chartAttributes->size(); ++chartID)
	{
		if (!ChartExists(chartID))
		{
			continue;
		}
		QSignalBlocker blocker(m_charts[chartID]);
		m_charts[chartID]->ResetSpan();
	}
}


void iAHistogramContainer::UpdateAttributeRangeAttitude()
{
	QVector<iAImageTreeNode const *> likes, hates;
	FindByAttitude(m_root, iAImageTreeNode::Liked, likes);
	FindByAttitude(m_root, iAImageTreeNode::Hated, hates);
	m_attitudes.clear();
	for (int chartID = 0; chartID != m_chartAttributes->size(); ++chartID)
	{
		m_attitudes.push_back(QVector<float>());
		if (!ChartExists(chartID))
		{
			continue;
		}
		auto numBin = m_charts[chartID]->GetNumBin();
		AttributeHistogram likeHist(numBin);
		GetHistData(likeHist, chartID, m_charts[chartID], likes, numBin, m_chartAttributeMapper);
		AttributeHistogram hateHist(numBin);
		GetHistData(hateHist, chartID, m_charts[chartID], hates, numBin, m_chartAttributeMapper);

		for (size_t b = 0; b < numBin; ++b)
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
			m_attitudes[chartID].push_back(attitude);
			m_charts[chartID]->SetBinColor(b, color);
			m_charts[chartID]->UpdateChart();
		}
	}
}


void iAHistogramContainer::ExportAttributeRangeRanking(QString const &fileName)
{
	QFile f(fileName);
	if (!f.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(nullptr, "GEMSe", "Couldn't open CSV file for writing attribute range rankings!");
		return;
	}
	QTextStream t(&f);
	for (int i = 0; i<m_attitudes.size(); ++i)
	{
		t << m_chartAttributes->at(i)->name();
		if (!ChartExists(i))
			continue;
		size_t numBin = m_charts[i]->GetNumBin();
		double min = m_chartAttributes->at(i)->min();
		double max = m_chartAttributes->at(i)->max();
		t << "," << min << "," << max << "," << numBin;
		for (int b = 0; b < m_attitudes[i].size(); ++b)
		{
			t << "," << m_attitudes[i][b];
		}
		t << "\n";
	}

}


qsizetype iAHistogramContainer::GetSelectedCount()
{
	return m_selected.size();
}


int iAHistogramContainer::GetSelectedChartID(qsizetype selectionIdx)
{
	return m_selected[selectionIdx];
}


void iAHistogramContainer::SetMarker(int chartID, double value)
{
	if (!ChartExists(chartID))
		return;
	m_charts[chartID]->SetMarker(value);
}


void iAHistogramContainer::RemoveMarker(int chartID)
{
	if (!ChartExists(chartID))
		return;
	m_charts[chartID]->RemoveMarker();
}


void iAHistogramContainer::ChartDblClicked()
{
	iAClusterAttribChart* slider = dynamic_cast<iAClusterAttribChart*>(sender());
	assert(slider);
	if (!slider)
	{
		LOG(lvlError, "ChartDblClicked called from non-slider widget.");
		return;
	}
	int chartID = slider->GetID();
	emit ChartDblClicked(chartID);
}


void iAHistogramContainer::FilterChanged(double min, double max)
{
	iAClusterAttribChart* slider = dynamic_cast<iAClusterAttribChart*>(sender());
	assert(slider);
	if (!slider)
	{
		LOG(lvlError, "FilterChanged called from non-slider widget.");
		return;
	}
	int chartID = slider->GetID();
	emit FilterChanged(chartID, min, max);
}


void iAHistogramContainer::SetSpanValues(int chartID, double min, double max)
{
	m_charts[chartID]->SetSpanValues(min, max);
}


QString iAHistogramContainer::GetSerializedHiddenCharts() const
{
	QStringList resultList;
	for (int id : m_disabledCharts)
	{
		resultList << QString::number(id);
	}
	return resultList.join(",");
}


void iAHistogramContainer::SetSerializedHiddenCharts(QString const & hiddenCharts)
{
	QStringList chartIDs = hiddenCharts.split(",");
	m_disabledCharts.clear();
	for (QString idStr : chartIDs)
	{
		bool ok;
		int id = idStr.toInt(&ok);
		if (ok)
		{
			m_disabledCharts.insert(id);
		}
	}
	CreateCharts();
}


void iAHistogramContainer::selectHistograms()
{
	QDialog dlg(this);
	QVBoxLayout* layout = new QVBoxLayout();
	QMap<int, QCheckBox*> boxes;

	QLabel *inputParams = new QLabel("Input Parameters");
	inputParams->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	layout->addWidget(inputParams);
	for (int i = 0; i < m_chartAttributes->size(); ++i)
	{
		if (m_chartAttributes->at(i)->min() == m_chartAttributes->at(i)->max() ||
			m_chartAttributes->at(i)->attribType() != iAAttributeDescriptor::Parameter)
		{
			continue;
		}
		auto box = new QCheckBox(m_chartAttributes->at(i)->name());
		box->setChecked(!m_disabledCharts.contains(i));
		boxes.insert(i, box);
		layout->addWidget(box);
	}
	QLabel * derivedOutput = new QLabel("Derived Output");
	derivedOutput->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	layout->addWidget(derivedOutput);
	for (int i = 0; i < m_chartAttributes->size(); ++i)
	{
		if (m_chartAttributes->at(i)->min() == m_chartAttributes->at(i)->max() ||
			m_chartAttributes->at(i)->attribType() != iAAttributeDescriptor::DerivedOutput)
		{
			continue;
		}
		auto box = new QCheckBox(m_chartAttributes->at(i)->name());
		box->setChecked(!m_disabledCharts.contains(i));
		boxes.insert(i, box);
		layout->addWidget(box);
	}
	auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
	layout->addWidget(buttons);
	dlg.setLayout(layout);
	if (dlg.exec() == QDialog::Accepted)
	{
		m_disabledCharts.clear();
		for (int key : boxes.keys())
		{
			if (!boxes[key]->isChecked())
			{
				m_disabledCharts.insert(key);
			}
		}
		CreateCharts();
	}
}
