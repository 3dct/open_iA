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

#include "iAFakeTreeNode.h"

iAFakeTreeNode::iAFakeTreeNode(iAITKIO::ImagePointer img, QString const & name) :
	iAImageTreeNode(), m_img(img),
	m_name(name)
{}

QString const & iAFakeTreeNode::GetName() const
{
	return m_name;
}

bool iAFakeTreeNode::IsLeaf() const
{
	return false;
}
int iAFakeTreeNode::GetChildCount() const
{
	return 0;
}
double iAFakeTreeNode::GetAttribute(int) const
{
	return 0;
}
 int iAFakeTreeNode::GetClusterSize() const
{
	return 0;
}
 int iAFakeTreeNode::GetFilteredSize() const
{
	return 0;
}

 ClusterImageType const iAFakeTreeNode::GetRepresentativeImage(int type, LabelImagePointer refImg) const
{
	return m_img;
}
 ClusterIDType iAFakeTreeNode::GetID() const
{
	return -1;
}
 void iAFakeTreeNode::GetMinMax(int chartID, double & min, double & max,
	iAChartAttributeMapper const & chartAttrMap) const
{}
 ClusterDistanceType iAFakeTreeNode::GetDistance() const
{
	return 0;
}
// we should never get into any of these:
 void iAFakeTreeNode::GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount)
{
	assert(false);
}
 void iAFakeTreeNode::SetParent(QSharedPointer<iAImageTreeNode > parent)
{
	assert(false);
}
 QSharedPointer<iAImageTreeNode > iAFakeTreeNode::GetParent() const
{
	assert(false);
	return QSharedPointer<iAImageTreeNode >();
}
 QSharedPointer<iAImageTreeNode > iAFakeTreeNode::GetChild(int idx) const
{
	assert(false);
	return QSharedPointer<iAImageTreeNode >();
}
 void iAFakeTreeNode::DiscardDetails()
{
	assert(false);
}
ClusterImageType const iAFakeTreeNode::GetLargeImage() const
{
	assert(false);
	return m_img;
}
 LabelPixelHistPtr iAFakeTreeNode::UpdateLabelDistribution() const
{
	assert(false);
	return LabelPixelHistPtr();
}
 CombinedProbPtr iAFakeTreeNode::UpdateProbabilities() const
{
	assert(false);
	return CombinedProbPtr();
}
 void iAFakeTreeNode::UpdateFilter(iAChartFilter const & filter,
	iAChartAttributeMapper const & chartAttrMap,
	iAResultFilter const & resultFilter)
{
	assert(false);
}
 void iAFakeTreeNode::GetSelection(QVector<QSharedPointer<iASingleResult> > & result) const
{
	
}
