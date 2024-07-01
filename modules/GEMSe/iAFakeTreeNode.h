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
	bool IsLeaf() const override;
	int GetChildCount() const override;
	double GetAttribute(int) const override;
	int GetClusterSize() const override;
	int GetFilteredSize() const override;
	ClusterImageType const GetRepresentativeImage(int type, LabelImagePointer refImg) const override;
	ClusterIDType GetID() const override;
	void GetMinMax(int chartID, double & min, double & max,
		iAChartAttributeMapper const & chartAttrMap) const override;
	ClusterDistanceType GetDistance() const override;
	// we should never get into any of these:
	void GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount) override;
	void SetParent(std::shared_ptr<iAImageTreeNode > parent) override;
	std::shared_ptr<iAImageTreeNode > GetParent() const override;
	std::shared_ptr<iAImageTreeNode > GetChild(int idx) const override;
	void DiscardDetails() const override;
	LabelPixelHistPtr UpdateLabelDistribution() const override;
	CombinedProbPtr UpdateProbabilities() const override;
	void UpdateFilter(iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttrMap,
		iAResultFilter const & resultFilter) override;
	void GetSelection(QVector<std::shared_ptr<iASingleResult> > & result) const override;

private:
	QString m_name;
};
