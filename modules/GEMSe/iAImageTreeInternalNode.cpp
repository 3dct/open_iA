// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageTreeInternalNode.h"

#include "iAGEMSeConstants.h" // for iARepresentativeType -> move to iARepresentative?
#include "iARepresentative.h"

#include <iALog.h>
#include <iAMathUtility.h>
#include <iAToolsITK.h>

#include <QFileInfo>

#include <random>

const int CHILD_NODE_NUMBER = 2;

iAImageTreeInternalNode::iAImageTreeInternalNode(
	std::shared_ptr<iAImageTreeNode > a,
	std::shared_ptr<iAImageTreeNode > b,
	LabelPixelType differenceMarkerValue,
	QString const & cachePath,
	ClusterIDType id,
	ClusterDistanceType distance
) :
	m_ID(id),
	m_clusterSize(0),
	m_filteredRepresentativeOutdated(true),
	m_differenceMarkerValue(differenceMarkerValue),
	m_children(a, b),
	m_distance(distance),
	m_cachePath(cachePath)
{
	for (int i = 0; i<GetChildCount(); ++i)
	{
		m_clusterSize += GetChild(i)->GetClusterSize();
	}
	m_filteredSize = m_clusterSize;

	QFileInfo fi(GetCachedFileName(iARepresentativeType::Difference));
	if (!fi.exists())
	{
		CalculateRepresentative(iARepresentativeType::Difference, LabelImagePointer());
		DiscardDetails();
	}
}


int iAImageTreeInternalNode::GetChildCount() const
{
	// TODO: check if something other than binary tree makes more sense
	return CHILD_NODE_NUMBER;
}


ClusterImageType iAImageTreeInternalNode::CalculateRepresentative(int type, LabelImagePointer refImg) const
{
	switch (type)
	{
	case iARepresentativeType::Difference:
	{
		QVector<iAITKIO::ImagePointer> imgs;
		for (int i = 0; i < GetChildCount(); ++i)
		{
			imgs.push_back(GetChild(i)->GetRepresentativeImage(type, refImg));
		}
		iAITKIO::ImagePointer rep = CalculateDifferenceMarkers(imgs, m_differenceMarkerValue);
		if (m_representative.size() == 0)
		{
			m_representative.push_back(rep);
		}
		else
		{
			m_representative[iARepresentativeType::Difference] = rep;
		}
		for (int i = 0; i < GetChildCount(); ++i)
		{
			GetChild(i)->DiscardDetails();
		}
		storeImage(m_representative[iARepresentativeType::Difference], GetCachedFileName(type), true);
		return m_representative[iARepresentativeType::Difference];
	}
	case iARepresentativeType::LabelDistribution:
		UpdateLabelDistribution();
		//storeImage(m_representative[iARepresentativeType::LabelDistribution], GetCachedFileName(type), true);
		return m_representative[iARepresentativeType::LabelDistribution];

	case iARepresentativeType::AverageEntropy:
	{
		CombinedProbPtr result = UpdateProbabilities();
		if (result->prob.size() == 0)
			return ClusterImageType();
		//storeImage(m_representative[iARepresentativeType::AverageEntropy], GetCachedFileName(type), true);
		return m_representative[iARepresentativeType::AverageEntropy];
	}
	case iARepresentativeType::Correctness:
	{
		if (!refImg)
		{
			return ClusterImageType();
		}
		ClusterImageType diffRep = GetRepresentativeImage(iARepresentativeType::Difference, LabelImagePointer());
		ClusterImageType correctnessImg = allocateImage(diffRep);
		auto diffImg = dynamic_cast<LabelImageType*>(diffRep.GetPointer());
		auto corrImg = dynamic_cast<LabelImageType*>(correctnessImg.GetPointer());
		itk::ImageRegionIterator<LabelImageType> refIt(refImg, refImg->GetLargestPossibleRegion());
		itk::ImageRegionIterator<LabelImageType> diffIt(diffImg, diffImg->GetLargestPossibleRegion());
		itk::ImageRegionIterator<LabelImageType> corrIt(corrImg, corrImg->GetLargestPossibleRegion());
		refIt.GoToBegin(); diffIt.GoToBegin(); corrIt.GoToBegin();
		while (!refIt.IsAtEnd())
		{
			if (refIt.Get() == diffIt.Get())
			{
				corrIt.Set(1);
			}
			else if (diffIt.Get() == m_labelCount)
			{
				corrIt.Set(m_labelCount);
			}
			else
			{
				corrIt.Set(0);
			}
			++refIt;
			++diffIt;
			++corrIt;
		}

		if (m_representative.size() <= iARepresentativeType::Correctness)
		{
			m_representative.resize(iARepresentativeType::Correctness + 1);
		}
		m_representative[iARepresentativeType::Correctness] = correctnessImg;
		storeImage(m_representative[iARepresentativeType::Correctness], GetCachedFileName(iARepresentativeType::Correctness), true);

		return m_representative[iARepresentativeType::Correctness];
	}
	case iARepresentativeType::AverageLabel:
	{
		CombinedProbPtr result = UpdateProbabilities();
		if (result->prob.size() == 0)
			return ClusterImageType();
		return m_representative[iARepresentativeType::AverageLabel];
	}
	default:
		LOG(lvlError, "Requested to calculate invalid representative type!");
		return ClusterImageType();
	}
}

ClusterImageType iAImageTreeInternalNode::CalculateFilteredRepresentative(int type, LabelImagePointer refImg) const
{
	switch (type)
	{
	case iARepresentativeType::Difference:
	{
		QVector<iAITKIO::ImagePointer> imgs;
		for (int i = 0; i < GetChildCount(); ++i)
		{
			// TODO: redundant to  CalculateRepresentative!
			if (GetChild(i)->GetRepresentativeImage(type, refImg))
			{
				imgs.push_back(GetChild(i)->GetRepresentativeImage(type, refImg));
			}
		}
		m_filteredRepresentative[type] =
			CalculateDifferenceMarkers(imgs, m_differenceMarkerValue);
		return m_filteredRepresentative[type];
	}
	case iARepresentativeType::AverageEntropy:
		// sum child probabilities

	case iARepresentativeType::LabelDistribution:
		// sum child label histograms


	default:
		LOG(lvlError, "Requested to calculate invalid filtered representative type!");
		return ClusterImageType();
	}
}

int iAImageTreeInternalNode::GetClusterSize() const
{
	return m_clusterSize;
}


void iAImageTreeInternalNode::GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount)
{
	if (GetFilteredSize() == 0)
	{
		return;
	}
	assert(amount >= 1);
	if (amount > 1)
	{
		int amountLeft = amount;
		for (int i = 0; i<GetChildCount(); ++i)
		{
			int curAmount =
				// we want to get amount/2 from each child (in ideal balanced tree), but child count is limit
				static_cast<int>(std::round(static_cast<double>(amount) * GetChild(i)->GetFilteredSize() / GetFilteredSize()));
			// take at least one from each child cluster (if it contains any non-filtered),
			// and never all from one (except if it contains all available items because of filtering)
			curAmount = clamp(
				(GetChild(i)->GetFilteredSize() == 0) ? 0 : 1,
				std::min(
					amountLeft,
					// rule only works for binary tree:
					(GetFilteredSize() == GetChild(i)->GetFilteredSize()) ? amount : amount - 1
				),
				curAmount);
			//int sizeBefore = result.size();
			if (curAmount == 0)
			{
				continue;
			}
			GetChild(i)->GetExampleImages(result, curAmount);
			//int imagesReturned = result.size() - sizeBefore;
			amountLeft -= curAmount;
		}
	}
	else
	{
		// choose a (random) representative from one child
		// TODO: explore selection strategies!
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, GetChildCount() - 1);
		int childID = dis(gen);
		if (GetChild(childID)->GetFilteredSize() == 0) // change if to while if more than 2 childs!
		{
			childID = (childID + 1) % 2;
		}
		GetChild(childID)->GetExampleImages(result, 1);
	}
}


ClusterImageType const iAImageTreeInternalNode::GetRepresentativeImage(int type, LabelImagePointer refImg) const
{
	if (GetFilteredSize() != GetClusterSize())
	{
		if (m_filteredRepresentative.size() <= type)
		{
			m_filteredRepresentative.resize(type + 1);
		}
		if (m_filteredRepresentativeOutdated || !m_filteredRepresentative[type])
		{
			RecalculateFilteredRepresentative(type, refImg);
		}
		/*
		// fine, this just means that all images were filtered out!
		if (!m_filteredRepresentative[type])
		{
		LOG(lvlError, "Filtered representative is nullptr!");
		}
		*/
		return m_filteredRepresentative[type];
	}
	if (m_representative.size() <= type)
	{
		m_representative.resize(type + 1);
	}
	if (!m_representative[type])
	{
		QFileInfo fi(GetCachedFileName(type));
		if (!fi.exists())
		{
			m_representative[type] = CalculateRepresentative(type, refImg);

		}
		else
		{
			iAITKIO::PixelType pixelType;
			iAITKIO::ScalarType scalarType;
			m_representative[type] = iAITKIO::readFile(GetCachedFileName(type), pixelType, scalarType, false);
			assert(pixelType == iAITKIO::PixelType::SCALAR);
		}
	}
	if (!m_representative[type])
	{
		//LOG(lvlError, "Representative is nullptr!");
	}
	return m_representative[type];
}


void iAImageTreeInternalNode::DiscardDetails() const
{
	m_representative.clear();
}


void iAImageTreeInternalNode::DiscardFilterData()
{
	m_filteredRepresentative.clear();
}


std::shared_ptr<iAImageTreeNode > iAImageTreeInternalNode::GetChild(int idx) const
{
	return (idx == 0) ? m_children.first : m_children.second;
}


ClusterIDType iAImageTreeInternalNode::GetID() const
{
	return m_ID;
}


double iAImageTreeInternalNode::GetAttribute(int /*id*/) const
{
	assert(false);
	return 0.0;
}

int  iAImageTreeInternalNode::GetFilteredSize() const
{
	return m_filteredSize;
}

void iAImageTreeInternalNode::UpdateFilter(iAChartFilter const & filter,
	iAChartAttributeMapper const & chartAttrMap,
	iAResultFilter const & resultFilter)
{
	m_filteredSize = 0;
	for (int i = 0; i<GetChildCount(); ++i)
	{
		GetChild(i)->UpdateFilter(filter, chartAttrMap, resultFilter);
		GetChild(i)->DiscardDetails();
		m_filteredSize += GetChild(i)->GetFilteredSize();
	}
	m_filteredRepresentativeOutdated = true;
}

void iAImageTreeInternalNode::RecalculateFilteredRepresentative(int type, LabelImagePointer refImg) const
{
	m_filteredRepresentativeOutdated = false;
	if (GetFilteredSize() == GetClusterSize())
	{
		LOG(lvlError, "RecalculateFilteredRepresentative called without need (not filtered!)");
		// return;
	}
	m_filteredRepresentative[type] = CalculateFilteredRepresentative(type, refImg);
}


QString iAImageTreeInternalNode::GetCachedFileName(int type) const
{
	return m_cachePath + "/rep" + QString::number(m_ID) +
		((type != iARepresentativeType::Difference) ? "-" + QString::number(type) : "") +
		".mhd";
}

ClusterDistanceType iAImageTreeInternalNode::GetDistance() const
{
	return m_distance;
}

#include <itkAddImageFilter.h>
#include <itkDivideImageFilter.h>

LabelPixelHistPtr iAImageTreeInternalNode::UpdateLabelDistribution() const
{
	LabelPixelHistPtr result(new LabelPixelHistogram());
	LabelPixelHistPtr childResult1 = GetChild(0)->UpdateLabelDistribution();
	LabelPixelHistPtr childResult2 = GetChild(1)->UpdateLabelDistribution();

	LabelImagePointer child1Img = childResult1->hist.at(0);
	LabelImageType::SizeType size = child1Img->GetLargestPossibleRegion().GetSize();
	for (int l = 0; l < m_differenceMarkerValue; ++l)
	{
		auto addImgFilter = itk::AddImageFilter<LabelImageType>::New();
		addImgFilter->SetInput1(childResult1->hist.at(l));
		addImgFilter->SetInput2(childResult2->hist.at(l));
		addImgFilter->Update();
		result->hist.push_back(addImgFilter->GetOutput());
	}
	result->count = childResult1->count + childResult2->count;

	auto labelEntropy = createImage<ProbabilityImageType>(
		size,
		child1Img->GetSpacing()
		);
	LabelImageType::IndexType idx;

	double limit = -std::log(1.0 / m_differenceMarkerValue);
	double normalizeFactor = 1 / limit;

	for (idx[0] = 0; idx[0] >= 0 && static_cast<uint64_t>(idx[0]) < size[0]; ++idx[0])
	{	// >= 0 checks to prevent signed int overflow!
		for (idx[1] = 0; idx[1] >= 0 && static_cast<uint64_t>(idx[1]) < size[1]; ++idx[1])
		{
			for (idx[2] = 0; idx[2] >= 0 && static_cast<uint64_t>(idx[2]) < size[2]; ++idx[2])
			{
				double entropy = 0;
				for (int l = 0; l < m_differenceMarkerValue; ++l)
				{
					LabelImagePointer img = result->hist.at(l);
					double histValue = img->GetPixel(idx);
					if (histValue > 0) // to avoid infinity - we take 0, which is appropriate according to limit
					{
						double prob = histValue / result->count;
						entropy += (prob * std::log(prob));
					}
				}
				entropy = -entropy;
				entropy = clamp(0.0, limit, entropy);
				labelEntropy->SetPixel(idx, entropy * normalizeFactor);
			}
		}
	}

	if (m_representative.size() <= iARepresentativeType::LabelDistribution)
	{
		m_representative.resize(iARepresentativeType::LabelDistribution + 1);
	}
	m_representative[iARepresentativeType::LabelDistribution] = labelEntropy;
	// unify with GetRepresentative somehow
	storeImage(m_representative[iARepresentativeType::LabelDistribution], GetCachedFileName(iARepresentativeType::LabelDistribution), true);
	return result;
}

CombinedProbPtr iAImageTreeInternalNode::UpdateProbabilities() const
{
	/* TODO: caching? */
	CombinedProbPtr result(new CombinedProbability());
	CombinedProbPtr childResult1 = GetChild(0)->UpdateProbabilities();
	CombinedProbPtr childResult2 = GetChild(1)->UpdateProbabilities();
	if (childResult1->prob.size() == 0 || childResult2->prob.size() == 0)
	{
		return result;
	}

	auto child1Img = childResult1->prob.at(0);
	ProbabilityImageType::SizeType size = child1Img->GetLargestPossibleRegion().GetSize();
	for (int l = 0; l < m_differenceMarkerValue; ++l)
	{
		if (childResult1->prob.at(l) && childResult2->prob.at(l))
		{
			auto addImgFilter = itk::AddImageFilter<ProbabilityImageType>::New();
			addImgFilter->SetInput1(childResult1->prob.at(l));
			addImgFilter->SetInput2(childResult2->prob.at(l));
			addImgFilter->Update();
			result->prob.push_back(addImgFilter->GetOutput());
		}
		else
		{
			result->prob.push_back(ProbabilityImagePointer());
			result->count = 0;
			return result;
		}
	}
	result->count = childResult1->count + childResult2->count;

	auto averageEntropy = createImage<ProbabilityImageType>(
		size,
		child1Img->GetSpacing()
		);
	ProbabilityImageType::IndexType idx;

	LabelImagePointer averageLabel = createImage<LabelImageType>(
		size,
		child1Img->GetSpacing()
		);

	double limit = -std::log(1.0 / m_differenceMarkerValue);
	double normalizeFactor = 1 / limit;

	for (idx[0] = 0; idx[0] >= 0 && static_cast<uint64_t>(idx[0]) < size[0]; ++idx[0])
	{	// >= 0 checks to prevent signed int overflow!
		for (idx[1] = 0; idx[1] >= 0 && static_cast<uint64_t>(idx[1]) < size[1]; ++idx[1])
		{
			for (idx[2] = 0; idx[2] >= 0 && static_cast<uint64_t>(idx[2]) < size[2]; ++idx[2])
			{
				double entropy = 0;
				double probMax = -1;
				int label = -1;
				for (int l = 0; l < m_differenceMarkerValue; ++l)
				{
					auto img = result->prob.at(l);
					double probSum = img->GetPixel(idx);
					if (probSum > probMax)
					{
						label = l;
						probMax = probSum;
					}
					if (probSum > 0) // to avoid infinity - we take 0, which is appropriate according to limit
					{
						double prob = probSum / result->count;
						entropy += (prob * std::log(prob));
					}
				}
				entropy = -entropy;
				entropy = clamp(0.0, limit, entropy);
				averageEntropy->SetPixel(idx, entropy * normalizeFactor);
				averageLabel->SetPixel(idx, label);
			}
		}
	}

	if (m_representative.size() <= iARepresentativeType::AverageLabel)
	{
		m_representative.resize(iARepresentativeType::AverageLabel + 1);
	}
	m_representative[iARepresentativeType::AverageEntropy] = averageEntropy;
	m_representative[iARepresentativeType::AverageLabel] = averageLabel;
	// unify with GetRepresentative somehow
	storeImage(m_representative[iARepresentativeType::AverageEntropy], GetCachedFileName(iARepresentativeType::AverageEntropy), true);
	storeImage(m_representative[iARepresentativeType::AverageLabel], GetCachedFileName(iARepresentativeType::AverageLabel), true);
	return result;
}


void iAImageTreeInternalNode::ClearFilterData()
{
	DiscardFilterData();
	for (int i = 0; i < GetChildCount(); ++i)
	{
		GetChild(i)->ClearFilterData();
	}
}


void iAImageTreeInternalNode::GetMinMax(int chartID, double & min, double & max,
	iAChartAttributeMapper const & chartAttrMap) const
{
	for (int i = 0; i < GetChildCount(); ++i)
	{
		GetChild(i)->GetMinMax(chartID, min, max, chartAttrMap);
	}
}


void iAImageTreeInternalNode::GetSelection(QVector<std::shared_ptr<iASingleResult> > & result) const
{
	if (m_filteredSize == 0)	// shortcut
	{
		return;
	}
	for (int i = 0; i < GetChildCount(); ++i)
	{
		GetChild(i)->GetSelection(result);
	}
}
