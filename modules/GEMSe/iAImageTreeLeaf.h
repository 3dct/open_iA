/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
	double GetProbabilityValue(int l, int x, int y, int z) const;
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
