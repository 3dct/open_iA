/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAFilterChart.h"
#include "iAConsole.h"
#include "charts/iAPlotTypes.h"
#include "iAImageTreeLeaf.h"
#include "iALabelInfo.h"
#include "iAMathUtility.h"
#include "iAParamHistogramData.h"
#include "iASlicerMode.h"

#include <QLabel>
#include <QVBoxLayout>

int ProbabilityHistogramBinCount = 40;

QSharedPointer<iAParamHistogramData> CreateEmptyProbData(iAValueType type, double min, double max)
{
	QSharedPointer<iAParamHistogramData> result(new iAParamHistogramData(
		type == Discrete ? (max-min) : ProbabilityHistogramBinCount,
		min, max, false, type));
	return result;
}

namespace
{
	const int NumOfChartsShown = 5;
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
	m_charts.push_back(new iAFilterChart(this, "Entropy", m_entropyChartData,
		QSharedPointer<iANameMapper>(), true));
	m_charts[0]->Plots()[0]->setColor(QColor(64, 64, 64));

	// label distribution chart:
	for (int label = 0; label < m_labelInfo->count(); ++label)
	{
		m_labelDistributionChartData.push_back(CreateEmptyProbData(Discrete, 0, m_labelInfo->count()));
	}
	m_charts.push_back(new iAFilterChart(this, "Label Distribution", m_labelDistributionChartData[0],
		QSharedPointer<iANameMapper>(), true));
	m_charts[1]->Plots()[0]->setColor(m_labelInfo->GetColor(0));
	for (int label = 1; label < m_labelInfo->count(); ++label)
	{
		m_drawers.push_back(QSharedPointer<iAPlot>(
			new iABarGraphDrawer(m_labelDistributionChartData[label],
				m_labelInfo->GetColor(label), 2)));
		m_charts[1]->AddPlot(m_drawers[m_drawers.size()-1]);
	}

	// highest probability distribution charts:
	m_probChartStart = m_charts.size();
	for (int l = 0; l < std::min(NumOfChartsShown, labelInfo->count()); ++l)
	{
		m_probabilitiesChartData.push_back(CreateEmptyProbData(Continuous, 0, 1));
		m_charts.push_back(new iAFilterChart(this, QString("Probability Distribution Label %1").arg(l), m_probabilitiesChartData[l],
			QSharedPointer<iANameMapper>(), true));
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
	m_charts[1]->Plots()[0]->setColor(m_labelInfo->GetColor(0));
	for (int l = 0; l < m_drawers.size(); ++l)
	{
		m_drawers[l]->setColor(m_labelInfo->GetColor(l+1));
	}
}

void iAProbingWidget::SetSelectedNode(iAImageTreeNode const * node)
{
	m_selectedNode = node;
}

void iAProbingWidget::ProbeUpdate(int x, int y, int z, int mode)
{
	// entropy chart:
	m_entropyChartData->Reset();
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
		m_entropyChartData->AddValue(entropy);
	});

	// label distribution chart:
	for (int l = 0; l < m_labelDistributionChartData.size(); ++l)
	{
		m_labelDistributionChartData[l]->Reset();
	}
	itk::Index<3> idx;
	idx[0] = x; idx[1] = y; idx[2] = z;
	int valueCount = 0;
	VisitLeafs(m_selectedNode, [&](iAImageTreeLeaf const * leaf)
	{
		int label = dynamic_cast<LabelImageType*>(leaf->GetLargeImage().GetPointer())->GetPixel(idx);
		m_labelDistributionChartData[label]->AddValue(label);
		valueCount++;
	});
	m_charts[1]->SetYBounds(0, valueCount);

	// find the NumOfChartsShown highest probabilities
	for (int i = 0; i < m_probabilitiesChartData.size(); ++i)
	{
		m_probabilitiesChartData[i]->Reset();
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
			m_probabilitiesChartData[i]->AddValue(probValue);
		});
		m_charts[m_probChartStart+i]->SetXCaption(QString("Probability Distribution Label %1").arg(labelValue));
		m_charts[m_probChartStart+i]->Plots()[0]->setColor(m_labelInfo->GetColor(labelValue));
	}

	// redraw all charts:
	for (int i = 0; i < m_charts.size(); ++i)
	{
		if (i != 1)	// not for label distribution!
			m_charts[i]->ResetYBounds();
		m_charts[i]->redraw();
	}
}
