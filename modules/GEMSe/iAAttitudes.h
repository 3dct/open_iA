#pragma once

#include "iAImageTree.h"

#include <QVector>

class iAChartSpanSlider;
class iAChartAttributeMapper;

struct AttributeHistogram
{
	int * data;
	AttributeHistogram(int numBins);
	~AttributeHistogram();
	AttributeHistogram(const AttributeHistogram & other) = delete;
	AttributeHistogram & operator=(const AttributeHistogram & other) = delete;
};

void GetHistData(AttributeHistogram & hist,
	int chartID, iAChartSpanSlider* chart, QVector<iAImageClusterNode const *> const & nodes, int numBin,
	iAChartAttributeMapper const & chartAttrMap);

void FindByAttitude(iAImageClusterNode const * node, iAImageClusterNode::Attitude att, QVector<iAImageClusterNode const *> & nodeList);
