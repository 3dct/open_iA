// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageTree.h"

#include "iAImageTreeInternalNode.h"
#include "iAImageTreeLeaf.h"
#include "iASamplingResults.h"
#include "iASingleResult.h"

#include <iALog.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <cassert>

namespace
{
	QString LeafMarker("leaf");
	QString MergeMarker("merge");
}


iAImageTree::iAImageTree(std::shared_ptr<iAImageTreeNode > node, int labelCount):
m_root(node),
m_labelCount(labelCount)
{
}


void iAImageTree::WriteNode(QTextStream & out, std::shared_ptr<iAImageTreeNode > const node, int level)
{
	for (int l=0; l<level; ++l)
	{
		out << " ";
	}
	out << QString::number(node->GetID()) << " ";
	if (node->IsLeaf())
	{
		iAImageTreeLeaf* leaf = (iAImageTreeLeaf*)node.get();
		out << LeafMarker << " " << leaf->GetDatasetID();
	}
	else
	{
		out << MergeMarker << " " << QString::number(node->GetDistance());
	}
	out << Qt::endl;
	for (int c=0; c<node->GetChildCount(); ++c)
	{
		WriteNode(out, node->GetChild(c), level+1);
	}
}


bool iAImageTree::Store(QString const & fileName) const
{
	if (!m_root)
	{
		LOG(lvlError, "Root is null!");
		return false;
	}
	QFile file(fileName);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Opening clustering file '%1' for writing failed!").arg(fileName));
		return false;
	}
	QTextStream out(&file);
	WriteNode(out, m_root, 0);
	file.close();
	return true;
}

std::shared_ptr<iASingleResult> findResultWithID(QVector<std::shared_ptr<iASingleResult> > const & sampleResults, int id)
{
	for (int i=0; i<sampleResults.size(); ++i)
	{
		if (sampleResults[i]->id() == id)
		{
			return sampleResults[i];
		}
	}
	// shouldn't happen...
	assert(false);
	LOG(lvlError, QString("Result with requested id %1 was not found!").arg(id));
	return std::shared_ptr<iASingleResult>();
}


std::shared_ptr<iAImageTreeNode> iAImageTree::ReadNode(QTextStream & in,
	std::shared_ptr<QVector<std::shared_ptr<iASamplingResults>>> samplingResults,
	int labelCount,
	QString const & outputDirectory /*, int& lastClusterID*/)
{
	if (in.atEnd())
	{
		assert(false);
		LOG(lvlError, "Reading node in cluster file failed!");
		return std::shared_ptr<iAImageTreeNode>();
	}
	QString currentLine = in.readLine().trimmed();
	QStringList strs = currentLine.split(" ");
	bool isLeaf(strs[1] == LeafMarker);
	bool isNum = false;
	int id = strs[0].toInt(&isNum);
	if (!isNum)
	{
		LOG(lvlError, QString("Reading node: Invalid (non-integer) ID in cluster file, line: '%1'").arg(currentLine));
		return std::shared_ptr<iAImageTreeNode>();
	}
	if (isLeaf)
	{
		int datasetID = strs[2].toInt(&isNum);
		if (!isNum)
		{
			LOG(lvlError, QString("Reading node: Invalid (non-integer) dataset ID in cluster file, line: '%1'").arg(currentLine));
			return std::shared_ptr<iAImageTreeNode>();
		}
		auto sampleResults = samplingResults->at(datasetID)->members();
		auto result = findResultWithID(sampleResults, id);
		return std::make_shared<iAImageTreeLeaf>(result, labelCount);
	}
	else
	{
		float diff = strs[2].toFloat();
		std::shared_ptr<iAImageTreeNode> child1(iAImageTree::ReadNode(in, samplingResults, labelCount, outputDirectory/*, lastClusterID*/));
		std::shared_ptr<iAImageTreeNode> child2(iAImageTree::ReadNode(in, samplingResults, labelCount, outputDirectory/*, lastClusterID*/));
		std::shared_ptr<iAImageTreeNode> result(new iAImageTreeInternalNode(child1, child2,
			labelCount,
			outputDirectory,
			id, diff)
		);
		child1->SetParent(result);
		child2->SetParent(result);
		return result;
	}
}


std::shared_ptr<iAImageTree> iAImageTree::Create(QString const & fileName,
	std::shared_ptr<QVector<std::shared_ptr<iASamplingResults>>> samplingResults,
	int labelCount)
{
	QFile file(fileName);
	std::shared_ptr<iAImageTree> result;
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Opening clustering file '%1' for reading failed!").arg(fileName));
		return result;
	}
	QTextStream in(&file);
	QFileInfo fi(fileName);
	QString dir(fi.absolutePath()+"/representatives");
	QDir qdir;
	if (!qdir.mkpath(dir))
	{
		LOG(lvlError, "Can't create representative directory!");
	}
	qsizetype lastClusterID = -1;
	for (int i=0; i<samplingResults->size(); ++i)
	{
		lastClusterID = std::max(lastClusterID, samplingResults->at(i)->size());
	}
	result =  std::shared_ptr<iAImageTree>(new iAImageTree(ReadNode(in, samplingResults, labelCount,
		dir/*, lastClusterID */), labelCount));
	file.close();
	return result;
}

int iAImageTree::labelCount() const
{
	return m_labelCount;
}

void GetClusterMinMax(iAImageTreeNode const * node, int chartID, double & min, double & max,
	iAChartAttributeMapper const & chartAttrMap)
{
	min = std::numeric_limits<double>::max();
	max = std::numeric_limits<double>::lowest();
	node->GetMinMax(chartID, min, max, chartAttrMap);
}
