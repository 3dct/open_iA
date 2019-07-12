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
#include "iAImageTreeLeaf.h"

#include "iAChartAttributeMapper.h"
#include "iAChartFilter.h"
#include "iASingleResult.h"

#include <iAToolsITK.h>

iAImageTreeLeaf::iAImageTreeLeaf(QSharedPointer<iASingleResult> img, int labelCount) :
	m_singleResult(img),
	m_filtered(false),
	m_labelCount(labelCount)
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


ClusterImageType const iAImageTreeLeaf::GetRepresentativeImage(int type, LabelImagePointer refImg) const
{
	if (m_filtered)
	{
		return ClusterImageType();
	}
	return m_singleResult->GetLabelledImage();
}


void iAImageTreeLeaf::DiscardDetails() const
{
	m_singleResult->DiscardDetails();
}


QSharedPointer<iAImageTreeNode > iAImageTreeLeaf::GetChild(int idx) const
{
	// leaf node, no children -> null pointer
	return QSharedPointer<iAImageTreeNode >();
}


ClusterIDType iAImageTreeLeaf::GetID() const
{
	return m_singleResult->GetID();
}


ClusterImageType const iAImageTreeLeaf::GetLargeImage() const
{
	return m_singleResult->GetLabelledImage();
}


double iAImageTreeLeaf::GetAttribute(int id) const
{
	return m_singleResult->GetAttribute(id);
}


void iAImageTreeLeaf::SetAttribute(int id, double value)
{
	m_singleResult->SetAttribute(id, value);
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
	LabelImageType* img = dynamic_cast<LabelImageType*>(m_singleResult->GetLabelledImage().GetPointer());
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
	for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
	{
		for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
		{
			for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
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
	if (!m_singleResult->ProbabilityAvailable())
		return result;
	for (int i = 0; i < m_labelCount; ++i)
	{
		// TODO: probably very problematic regarding memory leaks!!!!!
		result->prob.push_back(dynamic_cast<ProbabilityImageType*>(m_singleResult->GetProbabilityImg(i).GetPointer()));
	}
	result->count = 1;
	return result;
}


double iAImageTreeLeaf::GetProbabilityValue(int l, int x, int y, int z) const
{
	if (!m_singleResult->ProbabilityAvailable())
		return 0;
	itk::Index<3> idx; idx[0] = x; idx[1] = y; idx[2] = z;
	// probably very inefficient - dynamic cast involved!
	return dynamic_cast<ProbabilityImageType*>(m_singleResult->GetProbabilityImg(l).GetPointer())->GetPixel(idx);
}


int iAImageTreeLeaf::GetDatasetID() const
{
	return m_singleResult->GetDatasetID();
}


QSharedPointer<iAAttributes> iAImageTreeLeaf::GetAttributes() const
{
	return m_singleResult->GetAttributes();
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

void iAImageTreeLeaf::GetSelection(QVector<QSharedPointer<iASingleResult> > & result) const
{
	if (!m_filtered)
	{
		result.push_back(m_singleResult);
	}
}