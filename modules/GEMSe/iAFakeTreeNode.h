// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageTreeNode.h"

class iAFakeTreeNode : public iAImageTreeNode
{
private:
	iAITKIO::ImagePointer m_img;
public:
	iAFakeTreeNode(iAITKIO::ImagePointer img, QString const & name);
	QString const & name() const;
	virtual bool IsLeaf() const;
	virtual int GetChildCount() const;
	virtual double GetAttribute(int) const;
	virtual int GetClusterSize() const;
	virtual int GetFilteredSize() const;
	virtual ClusterImageType const GetRepresentativeImage(int type, LabelImagePointer refImg) const;
	virtual ClusterIDType GetID() const;
	virtual void GetMinMax(int chartID, double & min, double & max,
		iAChartAttributeMapper const & chartAttrMap) const;
	virtual ClusterDistanceType GetDistance() const;
	// we should never get into any of these:
	virtual void GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount);
	virtual void SetParent(std::shared_ptr<iAImageTreeNode > parent);
	virtual std::shared_ptr<iAImageTreeNode > GetParent() const;
	virtual std::shared_ptr<iAImageTreeNode > GetChild(int idx) const;
	virtual void DiscardDetails() const;
	ClusterImageType const GetLargeImage() const;
	virtual LabelPixelHistPtr UpdateLabelDistribution() const;
	virtual CombinedProbPtr UpdateProbabilities() const;
	virtual void UpdateFilter(iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttrMap,
		iAResultFilter const & resultFilter);
	virtual void GetSelection(QVector<std::shared_ptr<iASingleResult> > & result) const;

private:
	QString m_name;
};
