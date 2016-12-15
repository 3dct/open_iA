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
#include "iAHistogramContainer.h"

#include "iAAttitudes.h"
#include "iAAttributes.h"
#include "iAAttributeDescriptor.h"
#include "iAChartAttributeMapper.h"
#include "iAChartFilter.h"
#include "iAChartSpanSlider.h"
#include "iAConsole.h"
#include "iAImageTree.h"
#include "iAParamHistogramData.h"
#include "iAQtCaptionWidget.h"

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
	QSharedPointer<iAAttributes> chartAttributes,
	iAChartAttributeMapper const & chartAttributeMapper,
	iAImageTreeNode const * root,
	QStringList const & pipelineNames):
	m_chartAttributes(chartAttributes),
	m_chartAttributeMapper(chartAttributeMapper),
	m_root(root),
	m_paramChartLayout(0),
	m_pipelineNames(pipelineNames)
{
	m_paramChartContainer = new QWidget();
	m_paramChartWidget = new QWidget();
	CreateGridLayout();
	SetCaptionedContent(m_paramChartContainer, "Input Parameters", m_paramChartWidget);
	m_chartContainer = new QSplitter();
	m_derivedOutputChartContainer = new QWidget();
	m_derivedOutputChartWidget = new QWidget();
	m_derivedOutputChartWidget->setLayout(new QHBoxLayout());
	m_derivedOutputChartWidget->layout()->setSpacing(ChartSpacing);
	m_derivedOutputChartWidget->layout()->setMargin(0);
	SetCaptionedContent(m_derivedOutputChartContainer, "Derived Output", m_derivedOutputChartWidget);
}

void iAHistogramContainer::CreateGridLayout()
{
	delete m_paramChartLayout;
	m_paramChartLayout = new QGridLayout();
	m_paramChartLayout->setSpacing(ChartSpacing);
	m_paramChartLayout->setMargin(0);
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
	m_paramChartLayout->addWidget(new QLabel(m_pipelineNames[paramChartRow]), paramChartRow, 0);
	int paramChartMaxCols = 0;
	int derivedOutMaxCols = 0;
	double maxValue = -1;
	for (int chartID = 0; chartID != m_chartAttributes->size(); ++chartID)
	{
		QSharedPointer<iAAttributeDescriptor> attrib = m_chartAttributes->at(chartID);
		if (attrib->GetMin() == attrib->GetMax() || m_disabledCharts.contains(chartID))
		{
			continue;
		}
		// maximum number of bins:
		//		- square root of number of values (https://en.wikipedia.org/wiki/Histogram#Number_of_bins_and_width)
		//      - adapting to width of histogram?
		//      - if discrete or categorical values: limit by range
		size_t maxBin = std::min(static_cast<size_t>(std::sqrt(m_root->GetClusterSize())), HistogramBinCount);
		int numBin = (attrib->GetMin() == attrib->GetMax()) ? 1 :
			(attrib->GetValueType() == iAValueType::Discrete || attrib->GetValueType() == iAValueType::Categorical) ?
			std::min(static_cast<size_t>(attrib->GetMax() - attrib->GetMin() + 1), maxBin) :
			maxBin;
		QSharedPointer<iAParamHistogramData> data = iAParamHistogramData::Create(
			m_root,
			chartID,
			attrib->GetValueType(),
			attrib->GetMin(),
			attrib->GetMax(),
			attrib->IsLogScale(),
			m_chartAttributeMapper,
			numBin);
		if (!data)
		{
			DEBUG_LOG(QString("ERROR: Creating chart #%1 data for attribute %2 failed!").arg(chartID)
				.arg(attrib->GetName()));
			continue;
		}
		if (attrib->GetAttribType() == iAAttributeDescriptor::Parameter)
		{
			maxValue = std::max(data->GetMaxValue(), maxValue);
		}
		m_charts.insert(chartID, new iAChartSpanSlider(attrib->GetName(), chartID, data,
			attrib->GetNameMapper()));

		connect(m_charts[chartID], SIGNAL(Toggled(bool)), this, SLOT(ChartSelected(bool)));
		connect(m_charts[chartID], SIGNAL(FilterChanged(double, double)), this, SLOT(FilterChanged(double, double)));
		connect(m_charts[chartID], SIGNAL(ChartDblClicked()), this, SLOT(ChartDblClicked()));

		if (attrib->GetAttribType() == iAAttributeDescriptor::Parameter)
		{
			QList<int> datasetIDs = m_chartAttributeMapper.GetDatasetIDs(chartID);
			if (!datasetIDs.contains(curMinDatasetID))
			{
				// alternative to GridLayout: Use combination of VBox and HBox layout?
				curMinDatasetID = datasetIDs[0];
				paramChartCol = 1;
				paramChartRow++;
				m_paramChartLayout->addWidget(new QLabel(m_pipelineNames[paramChartRow]), paramChartRow, 0);
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
			m_chartAttributes->at(i)->GetAttribType() == iAAttributeDescriptor::Parameter)
		{
			m_charts[i]->SetMaxYAxisValue(maxValue);
		}
	}
	setStretchFactor(0, paramChartMaxCols);
	setStretchFactor(1, derivedOutMaxCols);
}


void iAHistogramContainer::UpdateClusterChartData(QVector<QSharedPointer<iAImageTreeNode> > const & selection)
{
	for (int chartID = 0; chartID < m_chartAttributes->size(); ++chartID)
	{
		if (!ChartExists(chartID))
		{
			continue;
		}
		m_charts[chartID]->ClearClusterData();
		foreach(QSharedPointer<iAImageTreeNode> const node, selection)
		{
			QSharedPointer<iAAttributeDescriptor> attrib = m_chartAttributes->at(chartID);
			m_charts[chartID]->AddClusterData(iAParamHistogramData::Create(
				node.data(), chartID,
				attrib->GetValueType(),
				attrib->GetMin(),
				attrib->GetMax(),
				attrib->IsLogScale(),
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
			QSharedPointer<iAAttributeDescriptor> attrib = m_chartAttributes->at(chartID);
			m_charts[chartID]->SetFilteredClusterData(iAParamHistogramData::Create(
				selectedNode, chartID,
				attrib->GetValueType(),
				attrib->GetMin(),
				attrib->GetMax(),
				attrib->IsLogScale(),
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
		QSharedPointer<iAAttributeDescriptor> attrib = m_chartAttributes->at(chartID);
		m_charts[chartID]->SetFilteredData(iAParamHistogramData::Create(
			m_root, chartID,
			attrib->GetValueType(),
			attrib->GetMin(),
			attrib->GetMax(),
			attrib->IsLogScale(),
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
}


void iAHistogramContainer::ChartSelected(bool selected)
{
	iAChartSpanSlider* chart = dynamic_cast<iAChartSpanSlider*>(sender());

	int id = m_charts.key(chart);
	if (selected)
	{
		m_selected.push_back(id);
	}
	else
	{
		int idx = m_selected.indexOf(id);
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
		int numBin = m_charts[chartID]->GetNumBin();
		AttributeHistogram likeHist(numBin);
		GetHistData(likeHist, chartID, m_charts[chartID], likes, numBin, m_chartAttributeMapper);
		AttributeHistogram hateHist(numBin);
		GetHistData(hateHist, chartID, m_charts[chartID], hates, numBin, m_chartAttributeMapper);

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
		QMessageBox::warning(0, "GEMSe", "Couldn't open CSV file for writing attribute range rankings!");
		return;
	}
	QTextStream t(&f);
	for (int i = 0; i<m_attitudes.size(); ++i)
	{
		t << m_chartAttributes->at(i)->GetName();
		if (!ChartExists(i))
			continue;
		size_t numBin = m_charts[i]->GetNumBin();
		double min = m_chartAttributes->at(i)->GetMin();
		double max = m_chartAttributes->at(i)->GetMax();
		t << "," << min << "," << max << "," << numBin;
		for (int b = 0; b < m_attitudes[i].size(); ++b)
		{
			t << "," << m_attitudes[i][b];
		}
		t << "\n";
	}

}


int iAHistogramContainer::GetSelectedCount()
{
	return m_selected.size();
}


int iAHistogramContainer::GetSelectedChartID(int selectionIdx)
{
	return m_selected[selectionIdx];
}


void iAHistogramContainer::SetMarker(int chartID, double value)
{
	if (!ChartExists(chartID))
		return;
	m_charts[chartID]->SetMarker(value);
}


void iAHistogramContainer::ChartDblClicked()
{
	iAChartSpanSlider* slider = dynamic_cast<iAChartSpanSlider*>(sender());
	assert(slider);
	if (!slider)
	{
		DEBUG_LOG("ChartDblClicked called from non-slider widget.");
		return;
	}
	int chartID = slider->GetID();
	emit ChartDblClicked(chartID);
}


void iAHistogramContainer::FilterChanged(double min, double max)
{
	iAChartSpanSlider* slider = dynamic_cast<iAChartSpanSlider*>(sender());
	assert(slider);
	if (!slider)
	{
		DEBUG_LOG("FilterChanged called from non-slider widget.");
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


void iAHistogramContainer::SelectHistograms()
{
	QDialog dlg(this);
	QVBoxLayout* layout = new QVBoxLayout();
	QMap<int, QCheckBox*> boxes;

	QLabel *inputParams = new QLabel("Input Parameters");
	inputParams->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	layout->addWidget(inputParams);
	for (int i = 0; i < m_chartAttributes->size(); ++i)
	{
		if (m_chartAttributes->at(i)->GetMin() == m_chartAttributes->at(i)->GetMax() ||
			m_chartAttributes->at(i)->GetAttribType() != iAAttributeDescriptor::Parameter)
		{
			continue;
		}
		auto box = new QCheckBox(m_chartAttributes->at(i)->GetName());
		box->setChecked(!m_disabledCharts.contains(i));
		boxes.insert(i, box);
		layout->addWidget(box);
	}
	QLabel * derivedOutput = new QLabel("Derived Output");
	derivedOutput->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	layout->addWidget(derivedOutput);
	for (int i = 0; i < m_chartAttributes->size(); ++i)
	{
		if (m_chartAttributes->at(i)->GetMin() == m_chartAttributes->at(i)->GetMax() ||
			m_chartAttributes->at(i)->GetAttribType() != iAAttributeDescriptor::DerivedOutput)
		{
			continue;
		}
		auto box = new QCheckBox(m_chartAttributes->at(i)->GetName());
		box->setChecked(!m_disabledCharts.contains(i));
		boxes.insert(i, box);
		layout->addWidget(box);
	}
	auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, SIGNAL(accepted()), &dlg, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), &dlg, SLOT(reject()));
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
