// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAGEMSeConstants.h"

class iAChartAttributeMapper;
class iAChartFilter;
class iAImageTreeNode;
class iASamplingResults;

class QTextStream;

class iAImageTree
{
public:
	static QSharedPointer<iAImageTree> Create(
		QString const & fileName,
		QSharedPointer<QVector<QSharedPointer<iASamplingResults> > > samplings,
		int labelCount);
	iAImageTree(QSharedPointer<iAImageTreeNode >, int labelCount);
	QSharedPointer<iAImageTreeNode > m_root;
	bool Store(QString const & fileName) const;
	int labelCount() const;
private:
	static void WriteNode(QTextStream & out, QSharedPointer<iAImageTreeNode >, int level);
	static QSharedPointer<iAImageTreeNode> ReadNode(
		QTextStream & in,
		QSharedPointer<QVector<QSharedPointer<iASamplingResults> > > samplings,
		int labelCount,
		QString const & outputDirectory,
		int & lastClusterID);
	int m_labelCount;
};

void GetClusterMinMax(iAImageTreeNode const * node, int chartID, double & min, double & max,
	iAChartAttributeMapper const & chartAttrMap);
