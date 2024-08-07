// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageTreeNode.h"

#include <QVector>

class iAClusterAttribChart;
class iAChartAttributeMapper;

struct AttributeHistogram
{
	int * data;
	AttributeHistogram(size_t numBins);
	~AttributeHistogram();
	AttributeHistogram(const AttributeHistogram & other) = delete;
	AttributeHistogram & operator=(const AttributeHistogram & other) = delete;
};

void GetHistData(AttributeHistogram & hist,
	int chartID, iAClusterAttribChart* chart, QVector<iAImageTreeNode const *> const & nodes, size_t numBin,
	iAChartAttributeMapper const & chartAttrMap);

void FindByAttitude(iAImageTreeNode const * node, iAImageTreeNode::Attitude att, QVector<iAImageTreeNode const *> & nodeList);

void SetAttitude(iAImageTreeNode * root, QStringList ids, iAImageTreeNode::Attitude att);

void SetAttitudesFromRankingFile(QString const & fileName, iAImageTreeNode* root);
void ExportAttitudesToRankingFile(QString const & fileName, iAImageTreeNode const * root);
