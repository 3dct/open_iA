// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAChartFilter.h"	// try to avoid - but iAResultFilter is a template

#include <iAITKImageTypes.h>
#include <iAITKIO.h>

#include <vtkSmartPointer.h>

#include <QVector>

#include <memory>

class iAChartAttributeMapper;

// TODO: try to avoid this:
class iAImageTreeLeaf;
class iASingleResult;

typedef int ClusterIDType;
typedef float ClusterDistanceType;

using ClusterImageType = iAITKIO::ImagePointer;

class vtkImageData;

struct LabelPixelHistogram
{
	QVector<LabelImagePointer> hist;
	int count;
};

struct CombinedProbability
{
	QVector<ProbabilityImagePointer> prob;
	int count;
};

typedef std::shared_ptr<LabelPixelHistogram> LabelPixelHistPtr;
typedef std::shared_ptr<CombinedProbability> CombinedProbPtr;

class iAImageTreeNode
{
public:
	enum Attitude {
		NoPreference,
		Liked,
		Hated
	};
	iAImageTreeNode();
	virtual ~iAImageTreeNode();
	virtual int GetChildCount() const = 0;
	virtual int GetClusterSize() const = 0;
	virtual int GetFilteredSize() const = 0;
	virtual void UpdateFilter(iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttrMap,
		iAResultFilter const & resultFilter) = 0;
	virtual bool IsLeaf() const = 0;
	//! median image for this cluster:
	virtual ClusterImageType const GetRepresentativeImage(int type, LabelImagePointer refImg) const = 0;
	virtual vtkSmartPointer<vtkImageData> GetCorrectnessEntropyImage(LabelImagePointer refImg) const;
	virtual void DiscardDetails() const = 0;
	virtual void ClearFilterData();
	virtual ClusterIDType GetID() const = 0;
	virtual void GetExampleImages(QVector<iAImageTreeLeaf *> & result, int amount) = 0;
	virtual void SetParent(std::shared_ptr<iAImageTreeNode > parent);
	virtual std::shared_ptr<iAImageTreeNode > GetParent() const;
	virtual std::shared_ptr<iAImageTreeNode > GetChild(int idx) const = 0;
	virtual double GetAttribute(int) const = 0;
	virtual void GetMinMax(int chartID, double & min, double & max,
		iAChartAttributeMapper const & chartAttrMap) const = 0;
	virtual ClusterDistanceType GetDistance() const = 0;
	virtual Attitude GetAttitude() const;
	virtual void SetAttitude(Attitude att);
	virtual Attitude ParentAttitude() const;
	virtual LabelPixelHistPtr UpdateLabelDistribution() const = 0;
	virtual CombinedProbPtr UpdateProbabilities() const = 0;
	virtual void GetSelection(QVector<std::shared_ptr<iASingleResult> > & result) const = 0;
private:
	std::shared_ptr<iAImageTreeNode > m_parent;
	Attitude m_attitude;
};

void FindNode(iAImageTreeNode const * searched, QList<std::shared_ptr<iAImageTreeNode> > & path, std::shared_ptr<iAImageTreeNode> curCluster, bool & found);
std::shared_ptr<iAImageTreeNode> GetSibling(std::shared_ptr<iAImageTreeNode> node);


template<typename VisitorFn>
void VisitNodes(iAImageTreeNode const * node, VisitorFn visitor)
{
	visitor(node);
	for (int i = 0; i<node->GetChildCount(); ++i)
	{
		VisitNodes(node->GetChild(i).get(), visitor);
	}
}
