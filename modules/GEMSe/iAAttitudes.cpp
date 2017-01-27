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
#include "iAConsole.h"
#include "iAImageTreeLeaf.h"
#include "iAMathUtility.h"

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
	iAImageTreeNode const * node, int chartID, iAChartSpanSlider* chart, int numBin,
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
			DEBUG_LOG(QString("Invalid ID in rankings file (%1)!").arg(ids[i]));
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
		DEBUG_LOG("Couldn't open CSV file for reading rankings!");
		return;
	}
	QTextStream t(&f);
	QString likes = t.readLine();
	QStringList likeIDs = likes.split(",");
	if (likeIDs.size() == 0 || likeIDs[0] != "Liked")
	{
		DEBUG_LOG("Invalid rankings file format!");
		return;
	}
	QString hates = t.readLine();
	QStringList hateIDs = hates.split(",");
	if (hateIDs.size() == 0 || hateIDs[0] != "Disliked")
	{
		DEBUG_LOG("Invalid rankings file format!");
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
		DEBUG_LOG("Couldn't open CSV file for writing attribute range rankings!");
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
