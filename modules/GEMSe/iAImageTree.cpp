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
#include "pch.h"
#include "iAImageTree.h"

#include "iAConsole.h"
#include "iAImageTreeInternalNode.h"
#include "iAImageTreeLeaf.h"
#include "iASamplingResults.h"
#include "iASingleResult.h"

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


iAImageTree::iAImageTree(QSharedPointer<iAImageTreeNode > node, int labelCount):
m_root(node),
m_labelCount(labelCount)
{
}


void iAImageTree::WriteNode(QTextStream & out, QSharedPointer<iAImageTreeNode > const node, int level)
{
	for (int l=0; l<level; ++l)
	{
		out << " ";
	}
	out << QString::number(node->GetID()) << " ";
	if (node->IsLeaf())
	{
		iAImageTreeLeaf* leaf = (iAImageTreeLeaf*)node.data();
		out << LeafMarker << " " << leaf->GetDatasetID();
	}
	else
	{
		out << MergeMarker << " " << QString::number(node->GetDistance());
	}
	out << endl;
	for (int c=0; c<node->GetChildCount(); ++c)
	{
		WriteNode(out, node->GetChild(c), level+1);
	}
}


bool iAImageTree::Store(QString const & fileName) const
{
	if (!m_root)
	{
		DEBUG_LOG("Root is null!");
		return false;
	}
	QFile file(fileName);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Opening clustering file '%1' for writing failed!").arg(fileName));
		return false;
	}
	QTextStream out(&file);
	WriteNode(out, m_root, 0);
	file.close();
	return true;
}

QSharedPointer<iASingleResult> findResultWithID(QVector<QSharedPointer<iASingleResult> > const & sampleResults, int id)
{
	for (int i=0; i<sampleResults.size(); ++i)
	{
		if (sampleResults[i]->GetID() == id)
		{
			return sampleResults[i];
		}
	}
	// shouldn't happen...
	assert(false);
	DEBUG_LOG(QString("Result with requested id %1 was not found!").arg(id));
	return QSharedPointer<iASingleResult>();
}


QSharedPointer<iAImageTreeNode> iAImageTree::ReadNode(QTextStream & in,
	QSharedPointer<QVector<QSharedPointer<iASamplingResults> > >samplingResults,
	int labelCount,
	QString const & outputDirectory,
	int & lastClusterID)
{
	if (in.atEnd())
	{
		assert(false);
		DEBUG_LOG("Reading node in cluster file failed!");
		return QSharedPointer<iAImageTreeNode>();
	}
	QString currentLine = in.readLine().trimmed();
	QStringList strs = currentLine.split(" ");
	bool isLeaf(strs[1] == LeafMarker);
	bool isNum = false;
	int id = strs[0].toInt(&isNum);
	if (!isNum)
	{
		DEBUG_LOG(QString("Reading node: Invalid (non-integer) ID in cluster file, line: '%1'").arg(currentLine));
		return QSharedPointer<iAImageTreeNode>();
	}
	if (isLeaf)
	{
		int datasetID = strs[2].toInt(&isNum);
		if (!isNum)
		{
			DEBUG_LOG(QString("Reading node: Invalid (non-integer) dataset ID in cluster file, line: '%1'").arg(currentLine));
			return QSharedPointer<iAImageTreeNode>();
		}
		QVector<QSharedPointer<iASingleResult> > sampleResults = samplingResults->at(datasetID)->GetResults();
		QSharedPointer<iASingleResult> result = findResultWithID(sampleResults, id);
		return QSharedPointer<iAImageTreeNode>(new iAImageTreeLeaf(result, labelCount) );
	}
	else
	{
		float diff = strs[2].toFloat();
		QSharedPointer<iAImageTreeNode> child1(iAImageTree::ReadNode(in, samplingResults, labelCount, outputDirectory, lastClusterID));
		QSharedPointer<iAImageTreeNode> child2(iAImageTree::ReadNode(in, samplingResults, labelCount, outputDirectory, lastClusterID));
		QSharedPointer<iAImageTreeNode> result(new iAImageTreeInternalNode(child1, child2,
			labelCount,
			outputDirectory,
			id, diff)
		);
		child1->SetParent(result);
		child2->SetParent(result);
		return result;
	}
}


QSharedPointer<iAImageTree> iAImageTree::Create(QString const & fileName,
	QSharedPointer<QVector<QSharedPointer<iASamplingResults> > > samplingResults,
	int labelCount)
{
	QFile file(fileName);
	QSharedPointer<iAImageTree> result;
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Opening clustering file '%1' for reading failed!").arg(fileName));
		return result;
	}
	QTextStream in(&file);
	QFileInfo fi(fileName);
	QString dir(fi.absolutePath()+"/representatives");
	QDir qdir;
	if (!qdir.mkpath(dir))
	{
		DEBUG_LOG("Can't create representative directory!");
	}
	int lastClusterID = -1;
	for (int i=0; i<samplingResults->size(); ++i)
	{
		lastClusterID = std::max(lastClusterID, samplingResults->at(i)->size());
	}
	result =  QSharedPointer<iAImageTree>(new iAImageTree(ReadNode(in, samplingResults, labelCount,
		dir, lastClusterID), labelCount));
	file.close();
	return result;
}

int iAImageTree::GetLabelCount() const
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
