// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageTreeNode.h"

//! internal (i.e. non-leaf) tree node.
//! currently assumes that tree is not modified after creation!
class iAImageTreeInternalNode : public iAImageTreeNode
{
public:
	iAImageTreeInternalNode(
		std::shared_ptr<iAImageTreeNode > a,
		std::shared_ptr<iAImageTreeNode > b,
		LabelPixelType differenceMarkerValue,
		QString const & cachePath,
		ClusterIDType id,
		ClusterDistanceType distance);
	int GetChildCount() const override;
	int GetClusterSize() const override;
	int GetFilteredSize() const override;
	void UpdateFilter(iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttrMap,
		iAResultFilter const & resultFilter) override;
	bool IsLeaf() const override { return false; }
	ClusterImageType const GetRepresentativeImage(int type, LabelImagePointer refImg) const override;
	void DiscardDetails() const override;
	virtual void DiscardFilterData();
	void ClearFilterData() override;
	ClusterIDType GetID() const override;
	void GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount) override;
	std::shared_ptr<iAImageTreeNode > GetChild(int idx) const override;
	double GetAttribute(int) const override;
	void GetMinMax(int chartID, double & min, double & max,
		iAChartAttributeMapper const & chartAttrMap) const override;
	ClusterDistanceType GetDistance() const override;
	LabelPixelHistPtr UpdateLabelDistribution() const override;
	CombinedProbPtr UpdateProbabilities() const override;
	void GetSelection(QVector<std::shared_ptr<iASingleResult> > & result) const override;
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
	std::pair<std::shared_ptr<iAImageTreeNode>, std::shared_ptr<iAImageTreeNode> > m_children;
	ClusterDistanceType m_distance;
	QString m_cachePath;
	int m_labelCount;
};
