// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAttitudes.h"

#include "iAClusterAttribChart.h"
#include "iAChartAttributeMapper.h"
#include "iAImageTreeLeaf.h"

#include <iALog.h>
#include <iAMathUtility.h>

#include <QFile>
#include <QTextStream>

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
	iAImageTreeNode const * node, int chartID, iAClusterAttribChart* chart, int numBin,
	iAChartAttributeMapper const & chartAttrMap)
{
	VisitLeafs(node, [&](iAImageTreeLeaf const* leaf)
	{
		if (!chartAttrMap.GetDatasetIDs(chartID).contains(leaf->GetDatasetID()))
		{
			return;
		}
		int attributeID = chartAttrMap.GetAttributeID(chartID, leaf->GetDatasetID());
		double value = leaf->GetAttribute(attributeID);
		int bin = clamp(0, numBin - 1, static_cast<int>(chart->mapValueToBin(value)));
		hist.data[bin]++;
	});
}

// re-use existing histograms?
void GetHistData(AttributeHistogram & hist,
	int chartID, iAClusterAttribChart* chart, QVector<iAImageTreeNode const *> const & nodes, int numBin,
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
	VisitNodes(node, [&](iAImageTreeNode const * leaf)
	{
		if (leaf->GetAttitude() == att)
		{
			nodeList.push_back(leaf);
		}
	});
}



void SetAttitude(iAImageTreeNode * node, iAImageTreeNode::Attitude att, int id)
{
	if (node->GetID() == id)
	{
		node->SetAttitude(att);
	}
	for (int i = 0; i<node->GetChildCount(); ++i)
	{
		SetAttitude(node->GetChild(i).data(), att, id);
	}
}


void SetAttitude(iAImageTreeNode * root, QStringList ids, iAImageTreeNode::Attitude att)
{
	for (int i = 1; i < ids.size(); ++i)
	{
		bool ok;
		int id = ids[i].toInt(&ok);
		if (!ok)
		{
			LOG(lvlError, QString("Invalid ID in rankings file (%1)!").arg(ids[i]));
			return;
		}
		SetAttitude(root, att, id);
	}
}

void SetAttitudesFromRankingFile(QString const & fileName, iAImageTreeNode* root)
{
	QFile f(fileName);
	if (!f.open(QIODevice::ReadOnly))
	{
		LOG(lvlError, "Couldn't open CSV file for reading rankings!");
		return;
	}
	QTextStream t(&f);
	QString likes = t.readLine();
	QStringList likeIDs = likes.split(",");
	if (likeIDs.size() == 0 || likeIDs[0] != "Liked")
	{
		LOG(lvlError, "Invalid rankings file format!");
		return;
	}
	QString hates = t.readLine();
	QStringList hateIDs = hates.split(",");
	if (hateIDs.size() == 0 || hateIDs[0] != "Disliked")
	{
		LOG(lvlError, "Invalid rankings file format!");
		return;
	}

	SetAttitude(root, likeIDs, iAImageTreeNode::Liked);
	SetAttitude(root, hateIDs, iAImageTreeNode::Hated);
}


void ExportAttitudesToRankingFile(QString const & fileName, iAImageTreeNode const * root)
{
	QVector<iAImageTreeNode const *> likes, hates;
	FindByAttitude(root, iAImageTreeNode::Liked, likes);
	FindByAttitude(root, iAImageTreeNode::Hated, hates);

	QFile f(fileName);
	if (!f.open(QIODevice::WriteOnly))
	{
		LOG(lvlError, "Couldn't open CSV file for writing attribute range rankings!");
		return;
	}
	QTextStream t(&f);
	t << "Liked";
	for (int i = 0; i < likes.size(); ++i)
	{
		t << "," << likes[i]->GetID();
	}
	t << "\n";
	t << "Disliked";
	for (int i = 0; i < hates.size(); ++i)
	{
		t << "," << hates[i]->GetID();
	}
	t << "\n";
}
