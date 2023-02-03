// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageTreeNode.h"

//! internal (i.e. non-leaf) tree node.
//! currently assumes that tree is not modified after creation!
class iAImageTreeInternalNode : public iAImageTreeNode
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
		iAChartAttributeMapper const & chartAttrMap,
		iAResultFilter const & resultFilter);
	virtual bool IsLeaf() const { return false; }
	virtual ClusterImageType const GetRepresentativeImage(int type, LabelImagePointer refImg) const;
	virtual void DiscardDetails() const;
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
	virtual void GetSelection(QVector<QSharedPointer<iASingleResult> > & result) const;
private:
	void RecalculateFilteredRepresentative(int type, LabelImagePointer refImg) const;
	QString GetCachedFileName(int type) const;
	ClusterImageType CalculateRepresentative(int type, LabelImagePointer refImg) const;
	ClusterImageType CalculateFilteredRepresentative(int type, LabelImagePointer refImg) const;
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
