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
#include "iAProbingWidget.h"

#include "iAFilterChart.h"
#include "iAConsole.h"
#include "iAImageTreeLeaf.h"
#include "iALabelInfo.h"
#include "iAMathUtility.h"
#include "iAParamHistogramData.h"
#include "iASlicerMode.h"

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
	m_labelCount(labelInfo->count())
{
	QWidget* contentWidget = new QWidget();
	setFrameShape(QFrame::NoFrame);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setWidgetResizable(true);
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setSpacing(0);
	contentWidget->setLayout(layout);
	for (int l = 0; l < NumOfChartsShown; ++l)
	{
		m_chartData.push_back(CreateEmptyProbData(Continuous, 0, 1));
		m_charts.push_back(new iAFilterChart(this, QString("Probability %1").arg(l), m_chartData[l],
			QSharedPointer<iANameMapper>(), true));
	}
	m_labelInfo = labelInfo;
	m_chartData.push_back(CreateEmptyProbData(Continuous, 0, 1));
	m_charts.push_back(new iAFilterChart(this, "Entropy", m_chartData[NumOfChartsShown],
		QSharedPointer<iANameMapper>(), true));
	m_chartData.push_back(CreateEmptyProbData(Discrete, 0, m_labelCount));
	m_charts.push_back(new iAFilterChart(this, "Label Distribution", m_chartData[NumOfChartsShown +1],
		QSharedPointer<iANameMapper>(), true));
	for (int c = 0; c < m_charts.size(); ++c)
	{
		layout->addWidget(m_charts[c]);
	}
	contentWidget->setMinimumHeight((NumOfChartsShown +2) * 100);
	this->setWidget(contentWidget);
}

void iAProbingWidget::SetLabelInfo(iALabelInfo const * labelInfo)
{
	m_labelInfo = labelInfo;
}

void iAProbingWidget::SetSelectedNode(iAImageTreeNode const * node)
{
	m_selectedNode = node;
}

void iAProbingWidget::ProbeUpdate(int x, int y, int z, int mode)
{
	for (int i = 0; i < m_chartData.size(); ++i)
	{
		m_chartData[i]->Reset();
	}
	std::vector<std::pair<double, int> > probSumOfCharts;
	for (int l = 0; l < m_labelCount; ++l)
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
		{
			return first.first > second.first;
		}
	);
	for (int i = 0; i < NumOfChartsShown; ++i)
	{
		int labelValue = probSumOfCharts[i].second;
		VisitLeafs(m_selectedNode, [&](iAImageTreeLeaf const * leaf)
		{
			double probValue = leaf->GetProbabilityValue(labelValue, x, y, z);
			m_chartData[i]->AddValue(probValue);
		});
		m_charts[i]->SetXCaption(QString("Probability %1").arg(labelValue));
		m_charts[i]->GetPrimaryDrawer()->setColor(m_labelInfo->GetColor(labelValue));
	}
	double limit = -std::log(1.0 / m_labelCount);
	double normalizeFactor = 1 / limit;
	VisitLeafs(m_selectedNode, [&](iAImageTreeLeaf const * leaf)
	{
		double entropy = 0.0;
		for (int l = 0; l < m_labelCount; ++l)
		{
			double value = leaf->GetProbabilityValue(l, x, y, z);
			if (value > 0)
			{
				entropy += (value * std::log(value));
			}
		}
		entropy = clamp(0.0, 1.0, -entropy * normalizeFactor);
		m_chartData[NumOfChartsShown]->AddValue(entropy);
	});

	itk::Index<3> idx;
	idx[0] = x; idx[1] = y; idx[2] = z;
	VisitLeafs(m_selectedNode, [&](iAImageTreeLeaf const * leaf)
	{
		m_chartData[NumOfChartsShown +1]->AddValue(dynamic_cast<LabelImageType*>(leaf->GetLargeImage().GetPointer())->GetPixel(idx));
	});

	for (int i = 0; i < m_charts.size(); ++i)
	{
		m_charts[i]->ResetMaxYAxisValue();
		m_charts[i]->redraw();
	}
}
