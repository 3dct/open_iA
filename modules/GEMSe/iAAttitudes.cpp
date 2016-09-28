#include "iAAttitudes.h"

#include "iAChartSpanSlider.h"
#include "iAChartAttributeMapper.h"
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
	iAImageClusterNode const * node, int chartID, iAChartSpanSlider* chart, int numBin,
	iAChartAttributeMapper const & chartAttrMap)
{
	if (node->IsLeaf())
	{
		iAImageClusterLeaf* leaf = (iAImageClusterLeaf*)node;
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
	int chartID, iAChartSpanSlider* chart, QVector<iAImageClusterNode const *> const & nodes, int numBin,
	iAChartAttributeMapper const & chartAttrMap)
{
	for (int l = 0; l < nodes.size(); ++l)
	{
		iAImageClusterNode const * node = nodes[l];
		AddClusterData(hist, node, chartID, chart, numBin, chartAttrMap);
	}
}

void FindByAttitude(iAImageClusterNode const * node, iAImageClusterNode::Attitude att, QVector<iAImageClusterNode const *> & nodeList)
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
