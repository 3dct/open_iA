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
#include "iAProbingWidget.h"

#include "iAImageTreeLeaf.h"
#include "iALabelInfo.h"
#include "iAParamHistogramData.h"

#include <charts/iAChartWidget.h>
#include <charts/iAPlotTypes.h>
#include <iAConsole.h>
#include <iAMathUtility.h>
#include <iASlicerMode.h>

#include <QLabel>
#include <QVBoxLayout>


namespace
{
	const int NumOfChartsShown = 2;
	const int ProbabilityHistogramBinCount = 25;
	const int BarMargin = 2;
}

QSharedPointer<iAParamHistogramData> CreateEmptyProbData(iAValueType type, double min, double max)
{
	QSharedPointer<iAParamHistogramData> result(new iAParamHistogramData(
		type == Discrete ? (max-min) : ProbabilityHistogramBinCount,
		min, max, false, type));
	return result;
}

iAProbingWidget::iAProbingWidget(iALabelInfo const * labelInfo):
	m_labelInfo(labelInfo)
{
	QWidget* contentWidget = new QWidget();
	setFrameShape(QFrame::NoFrame);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setWidgetResizable(true);
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setSpacing(0);
	contentWidget->setLayout(layout);
	m_lbInfo = new QLabel("Probing n/a");
	m_lbInfo->setMaximumHeight(20);
	layout->addWidget(m_lbInfo);
	
	// entropy chart:
	m_entropyChartData = CreateEmptyProbData(Continuous, 0, 1);
	m_charts.push_back(new iAChartWidget(this, "Algorithmic Uncertainty", "Frequency (Members)"));
	auto plot = QSharedPointer<iAPlot>(
		new iABarGraphPlot(m_entropyChartData,
			QColor(117, 112, 179), BarMargin));
	m_charts[0]->addPlot(plot);

	// label distribution chart:
	for (int label = 0; label < m_labelInfo->count(); ++label)
	{
		m_labelDistributionChartData.push_back(CreateEmptyProbData(Discrete, 0, m_labelInfo->count()));
	}
	m_charts.push_back(new iAChartWidget(this, "Label", "Frequency (Members)"));
	for (int label = 0; label < m_labelInfo->count(); ++label)
	{
		m_drawers.push_back(QSharedPointer<iAPlot>(
			new iABarGraphPlot(m_labelDistributionChartData[label],
				m_labelInfo->color(label), BarMargin)));
		m_charts[1]->addPlot(m_drawers[m_drawers.size()-1]);
	}

	// highest probability distribution charts:
	m_probChartStart = m_charts.size();
	for (int l = 0; l < std::min(NumOfChartsShown, labelInfo->count()); ++l)
	{
		m_probabilitiesChartData.push_back(CreateEmptyProbData(Continuous, 0, 1));
		m_charts.push_back(new iAChartWidget(this, QString("Probability Label %1").arg(l), "Frequency (Members)"));
		auto plot = QSharedPointer<iAPlot>(
			new iABarGraphPlot(m_probabilitiesChartData[l],
				m_labelInfo->color(l), BarMargin));
		m_charts[m_charts.size() - 1]->addPlot(plot);
	}
	for (int c = 0; c < m_charts.size(); ++c)
	{
		layout->addWidget(m_charts[c]);
	}
	contentWidget->setMinimumHeight((m_charts.size()) * 100);
	this->setWidget(contentWidget);
}

void iAProbingWidget::SetLabelInfo(iALabelInfo const * labelInfo)
{
	m_labelInfo = labelInfo;
	for (int l = 0; l < m_drawers.size(); ++l)
	{
		m_drawers[l]->setColor(m_labelInfo->color(l));
	}
}

void iAProbingWidget::SetSelectedNode(iAImageTreeNode const * node)
{
	m_selectedNode = node;
}

void iAProbingWidget::ProbeUpdate(int x, int y, int z, int /*mode*/)
{
	// entropy chart:
	m_entropyChartData->reset();
	double limit = -std::log(1.0 / m_labelInfo->count());
	double normalizeFactor = 1 / limit;
	VisitLeafs(m_selectedNode, [&](iAImageTreeLeaf const * leaf)
	{
		double entropy = 0.0;
		for (int l = 0; l < m_labelInfo->count(); ++l)
		{
			double value = leaf->GetProbabilityValue(l, x, y, z);
			if (value > 0)
			{
				entropy += (value * std::log(value));
			}
		}
		entropy = clamp(0.0, 1.0, -entropy * normalizeFactor);
		m_entropyChartData->addValue(entropy);
	});

	// label distribution chart:
	for (int l = 0; l < m_labelDistributionChartData.size(); ++l)
	{
		m_labelDistributionChartData[l]->reset();
	}
	itk::Index<3> idx;
	idx[0] = x; idx[1] = y; idx[2] = z;
	int valueCount = 0;
	VisitLeafs(m_selectedNode, [&](iAImageTreeLeaf const * leaf)
	{
		int label = dynamic_cast<LabelImageType*>(leaf->GetLargeImage().GetPointer())->GetPixel(idx);
		m_labelDistributionChartData[label]->addValue(label);
		valueCount++;
	});
	m_charts[1]->setYBounds(0, valueCount);

	// find the NumOfChartsShown highest probabilities
	for (int i = 0; i < m_probabilitiesChartData.size(); ++i)
	{
		m_probabilitiesChartData[i]->reset();
	}
	std::vector<std::pair<double, int> > probSumOfCharts;
	for (int l = 0; l < m_labelInfo->count(); ++l)
	{
		double probSum = 0;
		VisitLeafs(m_selectedNode, [&](iAImageTreeLeaf const * leaf)
		{
			double probValue = leaf->GetProbabilityValue(l, x, y, z);
			probSum += probValue;
		});
		probSumOfCharts.push_back(std::make_pair(probSum, l));
	}

	std::sort(probSumOfCharts.begin(), probSumOfCharts.end(),
		[](std::pair<double, int> const & first, std::pair<double, int> second)
		{	// we need descending sort order
			return first.first > second.first;
		}
	);
	m_lbInfo->setText(QString("Probing %1, %2, %3;").arg(x).arg(y).arg(z));
	for (int i = 0; i < m_probabilitiesChartData.size(); ++i)
	{
		int labelValue = probSumOfCharts[i].second;
		VisitLeafs(m_selectedNode, [&](iAImageTreeLeaf const * leaf)
		{
			double probValue = leaf->GetProbabilityValue(labelValue, x, y, z);
			m_probabilitiesChartData[i]->addValue(probValue);
		});
		m_charts[m_probChartStart+i]->setXCaption(QString("Probability Distribution Label %1").arg(labelValue));
		m_charts[m_probChartStart+i]->plots()[0]->setColor(m_labelInfo->color(labelValue));
	}

	// redraw all charts:
	for (int i = 0; i < m_charts.size(); ++i)
	{
		if (i != 1)	// not for label distribution!
			m_charts[i]->resetYBounds();
		m_charts[i]->update();
	}
}
