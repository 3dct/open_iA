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
#include "iAAttitudes.h"

#include "iAChartSpanSlider.h"
#include "iAChartAttributeMapper.h"
#include "iAImageTreeLeaf.h"
#include "iAMathUtility.h"

AttributeHistogram::AttributeHistogram(int numBins) :
	data(new int[numBins])
{
	std::fill(data, data + numBins, 0);
}

AttributeHistogram::~AttributeHistogram()
{
	delete[] data;
}


void AddClusterData(AttributeHistogram & hist,
	iAImageTreeNode const * node, int chartID, iAChartSpanSlider* chart, int numBin,
	iAChartAttributeMapper const & chartAttrMap)
{
	if (node->IsLeaf())
	{
		iAImageTreeLeaf* leaf = (iAImageTreeLeaf*)node;
		if (!chartAttrMap.GetDatasetIDs(chartID).contains(leaf->GetDatasetID()))
		{
			return;
		}
		int attributeID = chartAttrMap.GetAttributeID(chartID, leaf->GetDatasetID());
		double value = node->GetAttribute(attributeID);
		int bin = clamp(0, numBin - 1, static_cast<int>(chart->mapValueToBin(value)));
		hist.data[bin]++;
	}
	else
	{
		for (int i = 0; i<node->GetChildCount(); ++i)
		{
			AddClusterData(hist, node->GetChild(i).data(), chartID, chart, numBin, chartAttrMap);
		}
	}
}

// re-use existing histograms?
void GetHistData(AttributeHistogram & hist,
	int chartID, iAChartSpanSlider* chart, QVector<iAImageTreeNode const *> const & nodes, int numBin,
	iAChartAttributeMapper const & chartAttrMap)
{
	for (int l = 0; l < nodes.size(); ++l)
	{
		iAImageTreeNode const * node = nodes[l];
		AddClusterData(hist, node, chartID, chart, numBin, chartAttrMap);
	}
}

void FindByAttitude(iAImageTreeNode const * node, iAImageTreeNode::Attitude att, QVector<iAImageTreeNode const *> & nodeList)
{
	if (node->GetAttitude() == att)
	{
		nodeList.push_back(node);
	}
	for (int i = 0; i<node->GetChildCount(); ++i)
	{
		FindByAttitude(node->GetChild(i).data(), att, nodeList);
	}
}
