// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFakeTreeNode.h"

iAFakeTreeNode::iAFakeTreeNode(iAITKIO::ImagePointer img, QString const & name) :
	iAImageTreeNode(), m_img(img),
	m_name(name)
{}

QString const & iAFakeTreeNode::name() const
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

 ClusterImageType const iAFakeTreeNode::GetRepresentativeImage(int /*type*/, LabelImagePointer /*refImg*/) const
{
	return m_img;
}
 ClusterIDType iAFakeTreeNode::GetID() const
{
	return -1;
}
 void iAFakeTreeNode::GetMinMax(int /*chartID*/, double & /*min*/, double & /*max*/,
	iAChartAttributeMapper const & /*chartAttrMap*/) const
{}
 ClusterDistanceType iAFakeTreeNode::GetDistance() const
{
	return 0;
}
// we should never get into any of these:
 void iAFakeTreeNode::GetExampleImages(QVector<iAImageTreeLeaf *> & /*result*/, int /*amount*/)
{
	assert(false);
}
 void iAFakeTreeNode::SetParent(QSharedPointer<iAImageTreeNode > /*parent*/)
{
	assert(false);
}
 QSharedPointer<iAImageTreeNode > iAFakeTreeNode::GetParent() const
{
	assert(false);
	return QSharedPointer<iAImageTreeNode >();
}
 QSharedPointer<iAImageTreeNode > iAFakeTreeNode::GetChild(int /*idx*/) const
{
	assert(false);
	return QSharedPointer<iAImageTreeNode >();
}
 void iAFakeTreeNode::DiscardDetails() const
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
 void iAFakeTreeNode::UpdateFilter(iAChartFilter const & /*filter*/,
	iAChartAttributeMapper const & /*chartAttrMap*/,
	iAResultFilter const & /*resultFilter*/)
{
	assert(false);
}
 void iAFakeTreeNode::GetSelection(QVector<QSharedPointer<iASingleResult> > & /*result*/) const
{

}
