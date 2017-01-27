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

#include "iAChartSpanSlider.h"
#include "iAConsole.h"
#include "iAImageTreeLeaf.h"
#include "iAParamHistogramData.h"
#include "iASlicerMode.h"

#include <QVBoxLayout>

int ProbabilityHistogramBinCount = 100;

QSharedPointer<iAParamHistogramData> CreateEmptyProbData()
{
	QSharedPointer<iAParamHistogramData> result(new iAParamHistogramData(ProbabilityHistogramBinCount, 0, 1, false, Continuous));
	result->AddValue(0);	// dummy value
	return result;
}

iAProbingWidget::iAProbingWidget(int labelCount):
	m_labelCount(labelCount)
{
	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);
	for (int l = 0; l < m_labelCount; ++l)
	{
		QSharedPointer<iAParamHistogramData> data = CreateEmptyProbData();
		m_charts.push_back(new iAChartSpanSlider(QString("Probability %1").arg(l), l, data,	0, false));
		layout->addWidget(m_charts[l]);
	}
}

void iAProbingWidget::SetSelectedNode(iAImageTreeNode const * node)
{
	m_selectedNode = node;
}

void iAProbingWidget::ProbeUpdate(int x, int y, int z, int mode)
{
	m_chartData.clear();
	DEBUG_LOG(QString("Updating probabilities for slicer mode %1, coord(%2, %3, %4)").arg(GetSlicerModeString(mode)).arg(x).arg(y).arg(z));
	for (int l = 0; l < m_labelCount; ++l)
	{
		m_chartData.push_back(CreateEmptyProbData());
		VisitLeafs(m_selectedNode, [&](iAImageTreeLeaf const * leaf)
		{
			m_chartData[l]->AddValue(leaf->GetProbabilityValue(l, x, y, z));
			m_charts[l]->ClearClusterData();
			m_charts[l]->AddClusterData(m_chartData[l]);
			m_charts[l]->UpdateChart();
		});
	}
}