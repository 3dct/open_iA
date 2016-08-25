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
 
#include "pch.h"
#include "iAImageTree.h"

#include "iAAttributeFilter.h"
#include "iASingleResult.h"
#include "iAConsole.h"
#include "iAMathUtility.h"
#include "iARepresentative.h"
#include "iAToolsITK.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <cassert>
#include <random>

iAImageClusterNode::iAImageClusterNode():
	m_attitude(NoPreference)
{
}

void iAImageClusterNode::SetParent(QSharedPointer<iAImageClusterNode > parent)
{
	m_parent = parent;
}
QSharedPointer<iAImageClusterNode > iAImageClusterNode::GetParent() const
{
	return m_parent;
}

iAImageClusterNode::Attitude iAImageClusterNode::GetAttitude() const
{
	return m_attitude;
}

iAImageClusterNode::Attitude iAImageClusterNode::ParentAttitude() const
{
	return GetParent() ?
		(GetParent()->GetAttitude() != NoPreference ?
			GetParent()->GetAttitude() :
			GetParent()->ParentAttitude()
		) :
		NoPreference;
}

void iAImageClusterNode::SetAttitude(Attitude att)
{
	m_attitude = att;
}

// Internal Node:


iAImageClusterInternal::iAImageClusterInternal(
	QSharedPointer<iAImageClusterNode > a,
	QSharedPointer<iAImageClusterNode > b,
	LabelPixelType differenceMarkerValue,
	QString const & cachePath,
	ClusterIDType id,
	ClusterDistanceType distance
):
	m_children(a, b),
	m_clusterSize(0),
	m_filteredRepresentativeOutdated(true),
	m_differenceMarkerValue(differenceMarkerValue),
	m_ID(id),
	m_distance(distance),
	m_cachePath(cachePath)
{
	for (int i=0; i<GetChildCount(); ++i)
	{
		m_clusterSize += GetChild(i)->GetClusterSize();
	}
	m_filteredSize = m_clusterSize;

	QFileInfo fi(GetCachedFileName(iARepresentativeType::Difference));
	if (!fi.exists())
	{
		CalculateRepresentative(iARepresentativeType::Difference);
		DiscardDetails();
	}
}


int iAImageClusterInternal::GetChildCount() const
{
	// TODO: check if something other than binary tree makes more sense
	return CHILD_NODE_NUMBER;
}


ClusterImageType iAImageClusterInternal::CalculateRepresentative(int type) const
{
	switch (type)
	{
		case iARepresentativeType::Difference:
		{
			QVector<iAITKIO::ImagePointer> imgs;
			for (int i = 0; i < GetChildCount(); ++i)
			{
				imgs.push_back(GetChild(i)->GetRepresentativeImage(type));
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
			StoreImage(m_representative[iARepresentativeType::Difference], GetCachedFileName(type), true);
			return m_representative[iARepresentativeType::Difference];
		}
		case iARepresentativeType::LabelDistribution:
			UpdateLabelDistribution();
			//StoreImage(m_representative[iARepresentativeType::LabelDistribution], GetCachedFileName(type), true);
			return m_representative[iARepresentativeType::LabelDistribution];

		case iARepresentativeType::AverageEntropy:
			UpdateProbabilities();
			//StoreImage(m_representative[iARepresentativeType::AverageEntropy], GetCachedFileName(type), true);
			return m_representative[iARepresentativeType::AverageEntropy];
		case iARepresentativeType::AverageLabel:
			UpdateProbabilities();
			return m_representative[iARepresentativeType::AverageLabel];
		default:
			DEBUG_LOG("Requested to calculate invalid representative type!\n");
			return ClusterImageType();
	}
}

ClusterImageType iAImageClusterInternal::CalculateFilteredRepresentative(int type) const
{
	switch (type)
	{
		case iARepresentativeType::Difference:
		{
			QVector<iAITKIO::ImagePointer> imgs;
			for (int i = 0; i < GetChildCount(); ++i)
			{
				// TODO: redundant to  CalculateRepresentative!
				if (GetChild(i)->GetRepresentativeImage(type))
				{
					imgs.push_back(GetChild(i)->GetRepresentativeImage(type));
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
			DEBUG_LOG("Requested to calculate invalid filtered representative type!\n");
			return ClusterImageType();
	}
}

int iAImageClusterInternal::GetClusterSize() const
{
	return m_clusterSize;
}


void iAImageClusterInternal::GetExampleImages(QVector<iAImageClusterLeaf *> & result, int amount)
{
	if (GetFilteredSize() == 0)
	{
		return;
	}
	assert (amount >= 1);
	if (amount > 1)
	{
		int amountLeft = amount;
		for (int i=0; i<GetChildCount(); ++i)
		{
			int curAmount =
				// we want to get amount/2 from each child (in ideal balanced tree), but child count is limit
				static_cast<int>(std::round(static_cast<double>(amount) * GetChild(i)->GetFilteredSize() / GetFilteredSize() ));
			// take at least one from each child cluster (if it contains any non-filtered),
			// and never all from one (except if it contains all available items because of filtering)
			curAmount = clamp(
				(GetChild(i)->GetFilteredSize() == 0) ? 0 : 1,
				std::min(
					amountLeft,
					// rule only works for binary tree:
					(GetFilteredSize() == GetChild(i)->GetFilteredSize())? amount : amount-1
				),
				curAmount);
			int sizeBefore = result.size();
			if (curAmount == 0)
				continue;
			GetChild(i)->GetExampleImages(result, curAmount);
			int imagesReturned = result.size() - sizeBefore;
			amountLeft -= curAmount;
		}
	}
	else
	{
		// choose a (random) representative from one child
		// TODO: explore selection strategies!
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, GetChildCount()-1);
		int childID = dis(gen);
		if (GetChild(childID)->GetFilteredSize() == 0) // change if to while if more than 2 childs!
		{
			childID = (childID + 1) % 2;
		}
		GetChild(childID)->GetExampleImages(result, 1);
	}
}


ClusterImageType const iAImageClusterInternal::GetRepresentativeImage(int type) const
{
	if (GetFilteredSize() != GetClusterSize())
	{
		if (m_filteredRepresentative.size() <= type)
		{
			m_filteredRepresentative.resize(type + 1);
		}
		if (m_filteredRepresentativeOutdated || !m_filteredRepresentative[type])
		{
			RecalculateFilteredRepresentative(type);
		}
		/*
		// fine, this just means that all images were filtered out!
		if (!m_filteredRepresentative[type])
		{
			DEBUG_LOG("Filtered representative is NULL!\n");
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
			m_representative[type] = CalculateRepresentative(type);

		}
		else
		{
			iAITKIO::ScalarPixelType pixelType;
			m_representative[type] = iAITKIO::readFile(GetCachedFileName(type), pixelType);
		}
	}
	if (!m_representative[type])
	{
		DEBUG_LOG("Representative is NULL!\n");
	}
	return m_representative[type];
}


void iAImageClusterInternal::DiscardDetails()
{
	m_representative.clear();
}


void iAImageClusterInternal::DiscardFilterData()
{
	m_filteredRepresentative.clear();
}


QSharedPointer<iAImageClusterNode > iAImageClusterInternal::GetChild(int idx) const
{
	return (idx == 0) ? m_children.first : m_children.second;
}


ClusterIDType iAImageClusterInternal::GetID() const
{
	return m_ID;
}


double iAImageClusterInternal::GetAttribute(AttributeID id) const
{
	assert(false);
	return 0.0;
}

int  iAImageClusterInternal::GetFilteredSize() const
{
	return m_filteredSize;
}

void iAImageClusterInternal::UpdateFilter(iAAttributeFilter const & filter)
{
	m_filteredSize = 0;
	for (int i=0; i<GetChildCount(); ++i)
	{
		GetChild(i)->UpdateFilter(filter);
		m_filteredSize += GetChild(i)->GetFilteredSize();
	}
	m_filteredRepresentativeOutdated = true;
}

void iAImageClusterInternal::RecalculateFilteredRepresentative(int type) const
{
	m_filteredRepresentativeOutdated = false;
	if (GetFilteredSize() == GetClusterSize())
	{
		DEBUG_LOG("RecalculateFilteredRepresentative called without need (not filtered!)\n");
		// return;
	}
	m_filteredRepresentative[type] = CalculateFilteredRepresentative(type);
}


QString iAImageClusterInternal::GetCachedFileName(int type) const
{
	return m_cachePath + "/rep" + QString::number(m_ID) +
		((type != iARepresentativeType::Difference)? "-" + QString::number(type): "") + 
		".mhd";
}

ClusterDistanceType iAImageClusterInternal::GetDistance() const
{
	return m_distance;
}

#include <itkAddImageFilter.h>
#include <itkDivideImageFilter.h>

LabelPixelHistPtr iAImageClusterInternal::UpdateLabelDistribution() const
{
	LabelPixelHistPtr result(new LabelPixelHistogram());
	LabelPixelHistPtr childResult1 = GetChild(0)->UpdateLabelDistribution();
	LabelPixelHistPtr childResult2 = GetChild(1)->UpdateLabelDistribution();

	LabelImagePointer img = childResult1->hist.at(0);
	LabelImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
	for (int l = 0; l < m_differenceMarkerValue; ++l)
	{
		typedef itk::AddImageFilter<LabelImageType> AddImgFilterType;
		AddImgFilterType::Pointer addImgFilter = AddImgFilterType::New();
		addImgFilter->SetInput1(childResult1->hist.at(l));
		addImgFilter->SetInput2(childResult2->hist.at(l));
		addImgFilter->Update();
		result->hist.push_back(addImgFilter->GetOutput());
	}
	result->count = childResult1->count + childResult2->count;

	ProbabilityImagePointer labelEntropy = CreateImage<ProbabilityImageType>(
		size,
		img->GetSpacing()
		);
	LabelImageType::IndexType idx;

	double limit = -std::log(1.0 / m_differenceMarkerValue);
	double normalizeFactor = 1 / limit;

	for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
	{
		for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
		{
			for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
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
	StoreImage(m_representative[iARepresentativeType::LabelDistribution], GetCachedFileName(iARepresentativeType::LabelDistribution), true);
	return result;
}

CombinedProbPtr iAImageClusterInternal::UpdateProbabilities() const
{
	/* TODO: caching? */
	CombinedProbPtr result(new CombinedProbability());
	CombinedProbPtr childResult1 = GetChild(0)->UpdateProbabilities();
	CombinedProbPtr childResult2 = GetChild(1)->UpdateProbabilities();

	ProbabilityImagePointer img = childResult1->prob.at(0);
	ProbabilityImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
	for (int l = 0; l < m_differenceMarkerValue; ++l)
	{
		if (childResult1->prob.at(l) && childResult2->prob.at(l))
		{
			typedef itk::AddImageFilter<ProbabilityImageType> AddImgFilterType;
			AddImgFilterType::Pointer addImgFilter = AddImgFilterType::New();
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

	ProbabilityImagePointer averageEntropy = CreateImage<ProbabilityImageType>(
		size,
		img->GetSpacing()
		);
	ProbabilityImageType::IndexType idx;

	LabelImagePointer averageLabel = CreateImage<LabelImageType>(
		size,
		img->GetSpacing()
		);

	double limit = -std::log(1.0 / m_differenceMarkerValue);
	double normalizeFactor = 1 / limit;

	for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
	{
		for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
		{
			for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
			{
				double entropy = 0;
				double probMax = -1;
				int label = -1;
				for (int l = 0; l < m_differenceMarkerValue; ++l)
				{
					ProbabilityImagePointer img = result->prob.at(l);
					double probSum = img->GetPixel(idx);
					if (probSum > probMax)
					{
						label = l;
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

	if (m_representative.size() <= iARepresentativeType::AverageEntropy)
	{
		m_representative.resize(iARepresentativeType::AverageEntropy + 1);
	}
	m_representative[iARepresentativeType::AverageEntropy] = averageEntropy;
	m_representative[iARepresentativeType::AverageLabel] = averageLabel;
	// unify with GetRepresentative somehow
	StoreImage(m_representative[iARepresentativeType::AverageEntropy], GetCachedFileName(iARepresentativeType::AverageEntropy), true);
	StoreImage(m_representative[iARepresentativeType::AverageLabel], GetCachedFileName(iARepresentativeType::AverageLabel), true);
	return result;
}


// Leaf Node:


iAImageClusterLeaf::iAImageClusterLeaf(QSharedPointer<iASingleResult> img, int labelCount):
	m_singleResult(img),
	m_filtered(false),
	m_labelCount(labelCount)
{
}


int iAImageClusterLeaf::GetChildCount() const
{
	return 0;
}

int iAImageClusterLeaf::GetClusterSize() const
{
	return 1;
}

void iAImageClusterLeaf::GetExampleImages(QVector<iAImageClusterLeaf *> & result, int amount)
{
	if (amount == 0)
	{
		return;
	}
	result.push_back(this);
}

#include "iAConnector.h"

ClusterImageType const iAImageClusterLeaf::GetRepresentativeImage(int type) const
{
	if (m_filtered)
	{
		return ClusterImageType();
	}
	return m_singleResult->GetLabelledImage();
}


void iAImageClusterLeaf::DiscardDetails()
{
	m_singleResult->DiscardDetails();
}


ClusterImageType const iAImageClusterLeaf::GetDetailImage() const
{
	return m_singleResult->GetLabelledImage();
}


QSharedPointer<iAImageClusterNode > iAImageClusterLeaf::GetChild(int idx) const
{
	// leaf node, no children -> null pointer
	return QSharedPointer<iAImageClusterNode >();
}


ClusterIDType iAImageClusterLeaf::GetID() const
{
	return m_singleResult->GetID();
}


ClusterImageType const iAImageClusterLeaf::GetLargeImage() const
{
	return m_singleResult->GetLabelledImage();
}


double iAImageClusterLeaf::GetAttribute(AttributeID id) const
{
	return m_singleResult->GetAttribute(id);
}


void iAImageClusterLeaf::SetAttribute(int id, double value)
{
	m_singleResult->SetAttribute(id, value);
}

int iAImageClusterLeaf::GetFilteredSize() const
{
	return (m_filtered)? 0 : 1;
}

void iAImageClusterLeaf::UpdateFilter(iAAttributeFilter const & filter)
{
	m_filtered = !filter.Matches(this);
}

ClusterDistanceType iAImageClusterLeaf::GetDistance() const
{
	return 0.0;
}

LabelPixelHistPtr iAImageClusterLeaf::UpdateLabelDistribution() const
{
	LabelPixelHistPtr result(new LabelPixelHistogram());
	// initialize
	LabelImageType* img = dynamic_cast<LabelImageType*>(m_singleResult->GetLabelledImage().GetPointer());
	LabelImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
	for (int l = 0; l < m_labelCount; ++l)
	{
		LabelImagePointer p = CreateImage<LabelImageType>(
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

CombinedProbPtr iAImageClusterLeaf::UpdateProbabilities() const
{
	CombinedProbPtr result(new CombinedProbability());
	for (int i = 0; i < m_labelCount; ++i)
	{
								// TODO: probably very problematic regarding memory leaks!!!!!
		result->prob.push_back(dynamic_cast<ProbabilityImageType*>(m_singleResult->GetProbabilityImg(i).GetPointer()));
	}
	return result;
}


// tree

iAImageTree::iAImageTree(QSharedPointer<iAImageClusterNode > node, int labelCount):
m_root(node),
m_labelCount(labelCount)
{
}


void iAImageTree::WriteNode(QTextStream & out, QSharedPointer<iAImageClusterNode > const node, int level)
{
	for (int l=0; l<level; ++l)
	{
		out << " ";
	}
	out << QString::number(node->GetID())
		<< (!node->IsLeaf()?QString(" ")+QString::number(node->GetDistance()):"")
		<< endl;
	for (int c=0; c<node->GetChildCount(); ++c)
	{
		WriteNode(out, node->GetChild(c), level+1);
	}
}


bool iAImageTree::Store(QString const & fileName) const
{
	if (!m_root)
	{
		DEBUG_LOG("Root is null!");
		return false;
	}
	QFile file(fileName);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Opening clustering file '%1' for writing failed!").arg(fileName));
		return false;
	}
	QTextStream out(&file);
	WriteNode(out, m_root, 0);
	file.close();
	return true;
}

QSharedPointer<iASingleResult> findResultWithID(QVector<QSharedPointer<iASingleResult> > const & sampleResults, int id)
{
	for (int i=0; i<sampleResults.size(); ++i)
	{
		if (sampleResults[i]->GetID() == id)
		{
			return sampleResults[i];
		}
	}
	// shouldn't happen...
	assert(false);
	DEBUG_LOG(QString("Result with requested id %1 was not found!").arg(id));
	return QSharedPointer<iASingleResult>();
}


QSharedPointer<iAImageClusterNode> iAImageTree::ReadNode(QTextStream & in,
	QVector<QSharedPointer<iASingleResult> > const & sampleResults,
	int labelCount,
	QString const & outputDirectory,
	int & lastClusterID)
{
	if (in.atEnd())
	{
		assert(false);
		DEBUG_LOG("Reading node in cluster file failed!");
		return QSharedPointer<iAImageClusterNode>();
	}
	QString currentLine = in.readLine().trimmed();
	bool isNum = false;
	float diff = 0.0;
	if (currentLine.contains(" "))
	{
		QStringList strs = currentLine.split(" ");
		currentLine = strs[0];
		diff = strs[1].toFloat();
	}
	int id = currentLine.toInt(&isNum);

	bool isLeaf = isNum && id < sampleResults.size();

	if (!isNum)
	{
		// old-style clustering file with * as merge node marker;
		id = lastClusterID++;
	}

	if (isLeaf)
	{
		QSharedPointer<iASingleResult> result = findResultWithID(sampleResults, id);
		return QSharedPointer<iAImageClusterNode>(new iAImageClusterLeaf(result, labelCount) );
	}
	else
	{
		QSharedPointer<iAImageClusterNode> child1(iAImageTree::ReadNode(in, sampleResults, labelCount, outputDirectory, lastClusterID));
		QSharedPointer<iAImageClusterNode> child2(iAImageTree::ReadNode(in, sampleResults, labelCount, outputDirectory, lastClusterID));
		QSharedPointer<iAImageClusterNode> result(new iAImageClusterInternal(child1, child2,
			labelCount,
			outputDirectory,
			id, diff)
		);
		child1->SetParent(result);
		child2->SetParent(result);
		return result;
	}
}


QSharedPointer<iAImageTree> iAImageTree::Create(QString const & fileName, QVector<QSharedPointer<iASingleResult> > const & sampleResults, int labelCount)
{
	QFile file(fileName);
	QSharedPointer<iAImageTree> result;
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Opening clustering file '%1' for reading failed!").arg(fileName));
		return result;
	}
	QTextStream in(&file);
	QFileInfo fi(fileName);
	QString dir(fi.absolutePath()+"/representatives");
	QDir qdir;
	if (!qdir.mkpath(dir))
	{
		DEBUG_LOG("Can't create representative directory!");
	}
	int lastClusterID = sampleResults.size();
	result =  QSharedPointer<iAImageTree>(new iAImageTree(ReadNode(in, sampleResults, labelCount,
		dir, lastClusterID), labelCount));
	file.close();
	return result;
}

int iAImageTree::GetLabelCount() const
{
	return m_labelCount;
}

void iAImageClusterLeaf::GetMinMax(AttributeID attribID, double & min, double & max) const
{
	if (m_filtered)
	{
		return;
	}
	double value = GetAttribute(attribID);
	if (value < min)
	{
		min = value;
	}
	if (value > max)
	{
		max = value;
	}
}

void iAImageClusterInternal::GetMinMax(AttributeID attribID, double & min, double & max) const
{
	for (int i = 0; i < GetChildCount(); ++i)
	{
		GetChild(i)->GetMinMax(attribID, min, max);
	}
}

void GetClusterMinMax(iAImageClusterNode const * node, AttributeID attribID, double & min, double & max)
{
	min = std::numeric_limits<double>::max();
	max = std::numeric_limits<double>::lowest();
	node->GetMinMax(attribID, min, max);
}
