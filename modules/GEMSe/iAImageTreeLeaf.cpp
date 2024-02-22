// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageTreeLeaf.h"

#include "iAChartAttributeMapper.h"
#include "iAChartFilter.h"
#include "iASingleResult.h"

#include <iAToolsITK.h>

iAImageTreeLeaf::iAImageTreeLeaf(std::shared_ptr<iASingleResult> img, int labelCount) :
	m_filtered(false),
	m_labelCount(labelCount),
	m_singleResult(img)
{
}


int iAImageTreeLeaf::GetChildCount() const
{
	return 0;
}

int iAImageTreeLeaf::GetClusterSize() const
{
	return 1;
}

void iAImageTreeLeaf::GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount)
{
	if (amount == 0)
	{
		return;
	}
	result.push_back(this);
}


ClusterImageType const iAImageTreeLeaf::GetRepresentativeImage(int /*type*/, LabelImagePointer /*refImg*/) const
{
	if (m_filtered)
	{
		return ClusterImageType();
	}
	return m_singleResult->labelImage();
}


void iAImageTreeLeaf::DiscardDetails() const
{
	m_singleResult->discardDetails();
}


std::shared_ptr<iAImageTreeNode> iAImageTreeLeaf::GetChild(int /*idx*/) const
{
	// leaf node, no children -> null pointer
	return std::shared_ptr<iAImageTreeNode >();
}


ClusterIDType iAImageTreeLeaf::GetID() const
{
	return m_singleResult->id();
}


ClusterImageType const iAImageTreeLeaf::GetLargeImage() const
{
	return m_singleResult->labelImage();
}


double iAImageTreeLeaf::GetAttribute(int id) const
{
	return m_singleResult->attribute(id);
}


void iAImageTreeLeaf::SetAttribute(int id, double value)
{
	m_singleResult->setAttribute(id, value);
}

int iAImageTreeLeaf::GetFilteredSize() const
{
	return (m_filtered) ? 0 : 1;
}

void iAImageTreeLeaf::UpdateFilter(iAChartFilter const & filter,
	iAChartAttributeMapper const & chartAttrMap,
	iAResultFilter const & resultFilter)
{
	m_filtered = !filter.Matches(this, chartAttrMap) ||
		!ResultFilterMatches(this, resultFilter);
}

ClusterDistanceType iAImageTreeLeaf::GetDistance() const
{
	return 0.0;
}

LabelPixelHistPtr iAImageTreeLeaf::UpdateLabelDistribution() const
{
	LabelPixelHistPtr result(new LabelPixelHistogram());
	// initialize
	LabelImageType* img = dynamic_cast<LabelImageType*>(m_singleResult->labelImage().GetPointer());
	LabelImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
	for (int l = 0; l < m_labelCount; ++l)
	{
		LabelImagePointer p = createImage<LabelImageType>(
			size,
			img->GetSpacing());
		result->hist.push_back(p);
	}
	// calculate actual histogram:
	LabelImageType::IndexType idx;
	for (idx[0] = 0; idx[0] >= 0 && static_cast<uint64_t>(idx[0]) < size[0]; ++idx[0])
	{	// >= 0 checks to prevent signed int overflow!
		for (idx[1] = 0;  idx[1] >= 0 && static_cast<uint64_t>(idx[1]) < size[1]; ++idx[1])
		{
			for (idx[2] = 0;  idx[2] >= 0 && static_cast<uint64_t>(idx[2]) < size[2]; ++idx[2])
			{
				int label = img->GetPixel(idx);
				result->hist.at(label)->SetPixel(idx, 1);
			}
		}
	}
	result->count = 1;
	return result;
}

#include "iASingleResult.h"

CombinedProbPtr iAImageTreeLeaf::UpdateProbabilities() const
{
	CombinedProbPtr result(new CombinedProbability());
	if (!m_singleResult->probabilityAvailable())
	{
		return result;
	}
	for (int i = 0; i < m_labelCount; ++i)
	{
		// TODO: probably very problematic regarding memory leaks!!!!!
		result->prob.push_back(dynamic_cast<ProbabilityImageType*>(m_singleResult->probabilityImg(i).GetPointer()));
	}
	result->count = 1;
	return result;
}


double iAImageTreeLeaf::GetProbabilityValue(int l, double x, double y, double z) const
{
	if (!m_singleResult->probabilityAvailable())
	{
		return 0;
	}
	double worldCoord[3] = { x, y, z };
	auto itkImg = m_singleResult->probabilityImg(l).GetPointer();
	auto idx = mapWorldCoordsToIndex(itkImg, worldCoord);
	// probably very inefficient - dynamic cast involved!
	return dynamic_cast<ProbabilityImageType*>(itkImg)->GetPixel(idx);
}


int iAImageTreeLeaf::GetDatasetID() const
{
	return m_singleResult->datasetID();
}


std::shared_ptr<iAAttributes> iAImageTreeLeaf::GetAttributes() const
{
	return m_singleResult->attributes();
}

void iAImageTreeLeaf::GetMinMax(int chartID, double & min, double & max,
	iAChartAttributeMapper const & chartAttrMap) const
{
	if (m_filtered)
	{
		return;
	}
	if (!chartAttrMap.GetDatasetIDs(chartID).contains(GetDatasetID()))
	{
		return;
	}
	int attributeID = chartAttrMap.GetAttributeID(chartID, GetDatasetID());
	double value = GetAttribute(attributeID);
	if (value < min)
	{
		min = value;
	}
	if (value > max)
	{
		max = value;
	}
}

void iAImageTreeLeaf::GetSelection(QVector<std::shared_ptr<iASingleResult> > & result) const
{
	if (!m_filtered)
	{
		result.push_back(m_singleResult);
	}
}
