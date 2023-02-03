// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAAttributes.h>

#include "iAImageTreeNode.h"

class iASingleResult;

class iAImageTreeLeaf : public iAImageTreeNode
{
public:
	iAImageTreeLeaf(QSharedPointer<iASingleResult> img, int labelCount);
	virtual int GetChildCount() const;
	virtual int GetClusterSize() const;
	virtual int GetFilteredSize() const;
	virtual void UpdateFilter(iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttrMap,
		iAResultFilter const & resultFilter);
	virtual ClusterImageType const GetRepresentativeImage(int type, LabelImagePointer refImg) const;
	virtual void DiscardDetails() const;
	ClusterImageType const GetLargeImage() const;
	virtual ClusterIDType GetID() const;
	virtual bool IsLeaf() const { return true; }
	virtual void GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount);
	virtual QSharedPointer<iAImageTreeNode > GetChild(int idx) const;
	virtual double GetAttribute(int) const;
	virtual void GetMinMax(int chartID, double & min, double & max,
		iAChartAttributeMapper const & chartAttrMap) const;
	virtual ClusterDistanceType GetDistance() const;
	void SetAttribute(int id, double value);
	virtual LabelPixelHistPtr UpdateLabelDistribution() const;
	virtual CombinedProbPtr UpdateProbabilities() const;
	double GetProbabilityValue(int l, double x, double y, double z) const;
	int GetDatasetID() const;
	QSharedPointer<iAAttributes> GetAttributes() const;
	virtual void GetSelection(QVector<QSharedPointer<iASingleResult> > & result) const;
private:
	bool m_filtered;
	int m_labelCount;
	QSharedPointer<iASingleResult> m_singleResult;
};

template<typename VisitorFn>
void VisitLeafs(iAImageTreeNode const * node, VisitorFn visitor)
{
	if (!node->IsLeaf())
	{
		for (int i = 0; i<node->GetChildCount(); ++i)
		{
			VisitLeafs(node->GetChild(i).data(), visitor);
		}
	}
	else
	{
		iAImageTreeLeaf const * leaf = dynamic_cast<iAImageTreeLeaf const *> (node);
		visitor(leaf);
	}
}
