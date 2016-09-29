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
#pragma once

#include "iAGEMSeConstants.h"
#include "iAImageTreeNode.h"

const int CHILD_NODE_NUMBER = 2;

class iASingleResult;
class iAImageTreeLeaf;

//! internal (i.e. non-leaf) tree node.
//! currently assumes that tree is not modified after creation!
class iAImageTreeInternalNode: public iAImageTreeNode
{
public:
	iAImageTreeInternalNode(
		QSharedPointer<iAImageTreeNode > a,
		QSharedPointer<iAImageTreeNode > b,
		LabelPixelType differenceMarkerValue,
		QString const & cachePath,
		ClusterIDType id,
		ClusterDistanceType distance);
	virtual int GetChildCount() const;
	virtual int GetClusterSize() const;
	virtual int GetFilteredSize() const;
	virtual void UpdateFilter(iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttrMap);
	virtual bool IsLeaf() const { return false; }
	virtual ClusterImageType const GetRepresentativeImage(int type) const;
	virtual void DiscardDetails();
	virtual void DiscardFilterData();
	virtual void ClearFilterData();
	virtual ClusterIDType GetID() const;
	virtual void GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount);
	virtual QSharedPointer<iAImageTreeNode > GetChild(int idx) const;
	virtual double GetAttribute(int) const;
	virtual void GetMinMax(int chartID, double & min, double & max,
		iAChartAttributeMapper const & chartAttrMap) const;
	virtual ClusterDistanceType GetDistance() const;
	virtual LabelPixelHistPtr UpdateLabelDistribution() const;
	virtual CombinedProbPtr UpdateProbabilities() const;
private:
	void RecalculateFilteredRepresentative(int type) const;
	QString GetCachedFileName(int type) const;
	ClusterImageType CalculateRepresentative(int type) const;
	ClusterImageType CalculateFilteredRepresentative(int type) const;
	ClusterIDType m_ID;
	int m_clusterSize;
	int m_filteredSize;
	mutable bool m_filteredRepresentativeOutdated;
	LabelPixelType m_differenceMarkerValue; // TODO: find way to get rid of this variable
	mutable QVector<ClusterImageType> m_representative;
	mutable QVector<ClusterImageType> m_filteredRepresentative;
	std::pair<QSharedPointer<iAImageTreeNode >, QSharedPointer<iAImageTreeNode > > m_children;
	ClusterDistanceType m_distance;
	QString m_cachePath;
	int m_labelCount;
};

class iAAttributes;

class iAImageTreeLeaf: public iAImageTreeNode
{
public:
	iAImageTreeLeaf(QSharedPointer<iASingleResult> img, int labelCount);
	virtual int GetChildCount() const;
	virtual int GetClusterSize() const;
	virtual int GetFilteredSize() const;
	virtual void UpdateFilter(iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttrMap);
	virtual ClusterImageType const GetRepresentativeImage(int type) const;
	virtual void DiscardDetails();
	virtual ClusterImageType const GetDetailImage() const;
	virtual ClusterIDType GetID() const;
	virtual bool IsLeaf() const { return true; }
	virtual void GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount);
	virtual QSharedPointer<iAImageTreeNode > GetChild(int idx) const;
	ClusterImageType const GetLargeImage() const;
	virtual double GetAttribute(int) const;
	virtual void GetMinMax(int chartID, double & min, double & max,
		iAChartAttributeMapper const & chartAttrMap) const;
	virtual ClusterDistanceType GetDistance() const;
	void SetAttribute(int id, double value);
	virtual LabelPixelHistPtr UpdateLabelDistribution() const;
	virtual CombinedProbPtr UpdateProbabilities() const;
	int GetDatasetID() const;
	QSharedPointer<iAAttributes> GetAttributes() const;
private:
	bool m_filtered;
	int m_labelCount;
	QSharedPointer<iASingleResult> m_singleResult;
};


class iAChartFilter;
class iASamplingResults;

class QTextStream;

class iAImageTree
{
public:
	static QSharedPointer<iAImageTree> Create(
		QString const & fileName,
		QVector<QSharedPointer<iASamplingResults> > const & samplings,
		int labelCount);
	iAImageTree(QSharedPointer<iAImageTreeNode >, int labelCount);
	QSharedPointer<iAImageTreeNode > m_root;
	bool Store(QString const & fileName) const;
	int GetLabelCount() const;
private:
	static void WriteNode(QTextStream & out, QSharedPointer<iAImageTreeNode >, int level);
	static QSharedPointer<iAImageTreeNode> ReadNode(
		QTextStream & in,
		QVector<QSharedPointer<iASamplingResults> > const & samplings,
		int labelCount,
		QString const & outputDirectory,
		int & lastClusterID);
	int m_labelCount;
};

void GetClusterMinMax(iAImageTreeNode const * node, int chartID, double & min, double & max,
	iAChartAttributeMapper const & chartAttrMap);
