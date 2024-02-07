// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageTreeNode.h"

#include <iALog.h>

iAImageTreeNode::iAImageTreeNode() :
	m_attitude(NoPreference)
{
}

iAImageTreeNode::~iAImageTreeNode()
{}

void iAImageTreeNode::SetParent(std::shared_ptr<iAImageTreeNode > parent)
{
	m_parent = parent;
}

std::shared_ptr<iAImageTreeNode > iAImageTreeNode::GetParent() const
{
	return m_parent;
}

iAImageTreeNode::Attitude iAImageTreeNode::GetAttitude() const
{
	return m_attitude;
}

iAImageTreeNode::Attitude iAImageTreeNode::ParentAttitude() const
{
	return GetParent() ?
		(GetParent()->GetAttitude() != NoPreference ?
			GetParent()->GetAttitude() :
			GetParent()->ParentAttitude()
			) :
		NoPreference;
}

void iAImageTreeNode::SetAttitude(Attitude att)
{
	m_attitude = att;
}


void iAImageTreeNode::ClearFilterData()
{
}


void FindNode(iAImageTreeNode const * searched, QList<std::shared_ptr<iAImageTreeNode> > & path, std::shared_ptr<iAImageTreeNode> curCluster, bool & found)
{
	path.push_back(curCluster);
	if (curCluster.get() != searched)
	{
		for (int i = 0; i<curCluster->GetChildCount() && !found; ++i)
		{
			FindNode(searched, path, curCluster->GetChild(i), found);
		}
		if (!found)
		{
			path.removeLast();
		}
	}
	else
	{
		found = true;
	}
}

std::shared_ptr<iAImageTreeNode> GetSibling(std::shared_ptr<iAImageTreeNode> node)
{
	std::shared_ptr<iAImageTreeNode> parent(node->GetParent());
	for (int i = 0; i<parent->GetChildCount(); ++i)
	{
		if (parent->GetChild(i) != node)
		{
			return parent->GetChild(i);
		}
	}
	return std::shared_ptr<iAImageTreeNode>();
}


#include <vtkImageData.h>
#include "iAToolsVTK.h"

vtkSmartPointer<vtkImageData> iAImageTreeNode::GetCorrectnessEntropyImage(LabelImagePointer refImg) const
{
	auto correctnessImg = dynamic_cast<LabelImageType*>(GetRepresentativeImage(iARepresentativeType::Correctness, refImg).GetPointer());
	auto entropyImg = dynamic_cast<ProbabilityImageType*>(GetRepresentativeImage(iARepresentativeType::AverageEntropy, refImg).GetPointer());
	if (!correctnessImg)
	{
		LOG(lvlError, "Correctness image not available!");
		return vtkSmartPointer<vtkImageData>();
	}
	if (!entropyImg)
	{
		LOG(lvlError, "Entropy image not available!");
		return vtkSmartPointer<vtkImageData>();
	}

	int dim[3];
	dim[0] = entropyImg->GetLargestPossibleRegion().GetSize(0);
	dim[1] = entropyImg->GetLargestPossibleRegion().GetSize(1);
	dim[2] = entropyImg->GetLargestPossibleRegion().GetSize(2);
	double spacing[3];
	spacing[0] = entropyImg->GetSpacing()[0];
	spacing[1] = entropyImg->GetSpacing()[1];
	spacing[2] = entropyImg->GetSpacing()[2];
	vtkSmartPointer<vtkImageData> correctnessEntropyImg = allocateImage(VTK_UNSIGNED_CHAR, dim, spacing, 4);

	itk::Index<3> idx;
	for (idx[0] = 0; idx[0] < dim[0]; ++idx[0])
	{
		for (idx[1] = 0; idx[1] < dim[1]; ++idx[1])
		{
			for (idx[2] = 0; idx[2] < dim[2]; ++idx[2])
			{
				int correctness = correctnessImg->GetPixel(idx);
				double entropy = entropyImg->GetPixel(idx);
				for (int i = 0; i < 3; ++i)
				{
					correctnessEntropyImg->SetScalarComponentFromDouble(idx[0], idx[1], idx[2], i, correctness * 255);
				}
				// entropy = 0 -> low uncertainty
				// entropy = 1 -> high uncertainty
				correctnessEntropyImg->SetScalarComponentFromDouble(idx[0], idx[1], idx[2], 3,
					(correctness == 0) ?
						((1-entropy)*255) : // where the algorithm was wrong, we want to highlight regions were it also was sure   about the result
						(entropy * 255) );  //                         right,												unsure
			}
		}
	}
	return correctnessEntropyImg;
}
