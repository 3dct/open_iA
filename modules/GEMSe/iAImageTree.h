// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAGEMSeConstants.h"

#include <memory>

class iAChartAttributeMapper;
class iAChartFilter;
class iAImageTreeNode;
class iASamplingResults;

class QTextStream;

class iAImageTree
{
public:
	static std::shared_ptr<iAImageTree> Create(
		QString const & fileName,
		std::shared_ptr<QVector<std::shared_ptr<iASamplingResults> > > samplings,
		int labelCount);
	iAImageTree(std::shared_ptr<iAImageTreeNode >, int labelCount);
	std::shared_ptr<iAImageTreeNode > m_root;
	bool Store(QString const & fileName) const;
	int labelCount() const;
private:
	static void WriteNode(QTextStream & out, std::shared_ptr<iAImageTreeNode >, int level);
	static std::shared_ptr<iAImageTreeNode> ReadNode(
		QTextStream & in,
		std::shared_ptr<QVector<std::shared_ptr<iASamplingResults> > > samplings,
		int labelCount,
		QString const & outputDirectory
		/*, int& lastClusterID*/);
	int m_labelCount;
};

void GetClusterMinMax(iAImageTreeNode const * node, int chartID, double & min, double & max,
	iAChartAttributeMapper const & chartAttrMap);
