// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAAttributes.h>

#include "iAImageTreeNode.h"

class iASingleResult;

class iAImageTreeLeaf : public iAImageTreeNode
{
public:
	iAImageTreeLeaf(std::shared_ptr<iASingleResult> img, int labelCount);
	int GetChildCount() const override;
	int GetClusterSize() const override;
	int GetFilteredSize() const override;
	void UpdateFilter(iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttrMap,
		iAResultFilter const & resultFilter) override;
	ClusterImageType const GetRepresentativeImage(int type, LabelImagePointer refImg) const override;
	void DiscardDetails() const override;
	ClusterImageType const GetLargeImage() const;
	ClusterIDType GetID() const override;
	bool IsLeaf() const  override { return true; }
	void GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount) override;
	std::shared_ptr<iAImageTreeNode > GetChild(int idx) const override;
	double GetAttribute(int) const override;
	void GetMinMax(int chartID, double & min, double & max,
		iAChartAttributeMapper const & chartAttrMap) const override;
	ClusterDistanceType GetDistance() const override;
	void SetAttribute(int id, double value);
	LabelPixelHistPtr UpdateLabelDistribution() const override;
	CombinedProbPtr UpdateProbabilities() const override;
	double GetProbabilityValue(int l, double x, double y, double z) const;
	int GetDatasetID() const;
	std::shared_ptr<iAAttributes> GetAttributes() const;
	void GetSelection(QVector<std::shared_ptr<iASingleResult> > & result) const override;
private:
	bool m_filtered;
	int m_labelCount;
	std::shared_ptr<iASingleResult> m_singleResult;
};

template<typename VisitorFn>
void VisitLeafs(iAImageTreeNode const * node, VisitorFn visitor)
{
	if (!node->IsLeaf())
	{
		for (int i = 0; i<node->GetChildCount(); ++i)
		{
			VisitLeafs(node->GetChild(i).get(), visitor);
		}
	}
	else
	{
		iAImageTreeLeaf const * leaf = dynamic_cast<iAImageTreeLeaf const *> (node);
		visitor(leaf);
	}
}
