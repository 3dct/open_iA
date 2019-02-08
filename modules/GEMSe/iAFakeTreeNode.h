/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAImageTreeNode.h"

class iAFakeTreeNode : public iAImageTreeNode
{
private:
	iAITKIO::ImagePointer m_img;
public:
	iAFakeTreeNode(iAITKIO::ImagePointer img, QString const & name);
	QString const & GetName() const;
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
	virtual void SetParent(QSharedPointer<iAImageTreeNode > parent);
	virtual QSharedPointer<iAImageTreeNode > GetParent() const;
	virtual QSharedPointer<iAImageTreeNode > GetChild(int idx) const;
	virtual void DiscardDetails() const;
	ClusterImageType const GetLargeImage() const;
	virtual LabelPixelHistPtr UpdateLabelDistribution() const;
	virtual CombinedProbPtr UpdateProbabilities() const;
	virtual void UpdateFilter(iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttrMap,
		iAResultFilter const & resultFilter);
	virtual void GetSelection(QVector<QSharedPointer<iASingleResult> > & result) const;

private:
	QString m_name;
};
