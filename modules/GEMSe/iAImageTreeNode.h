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

#include "iAChartFilter.h"	// try to avoid - but iAResultFilter is a template

#include <io/iAITKIO.h> // TODO: replace?

#include <itkImage.h>
#include <itkSmartPointer.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QVector>

class iAChartAttributeMapper;

// TODO: try to avoid this:
class iAImageTreeLeaf;
class iASingleResult;

typedef int ClusterIDType;
typedef float ClusterDistanceType;

// TODO: Replace with some other definition / template?
const int DIM = 3;
typedef int LabelPixelType;
typedef itk::Image<LabelPixelType, DIM> LabelImageType;
typedef LabelImageType::Pointer LabelImagePointer;

typedef double ProbabilityPixel;
typedef itk::Image<ProbabilityPixel, DIM> ProbabilityImageType;
typedef ProbabilityImageType::Pointer ProbabilityImagePointer;

typedef iAITKIO::ImagePointer ClusterImageType;

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

typedef QSharedPointer<LabelPixelHistogram> LabelPixelHistPtr;
typedef QSharedPointer<CombinedProbability> CombinedProbPtr;

class iAImageTreeNode
{
public:
	enum Attitude {
		NoPreference,
		Liked,
		Hated
	};
	iAImageTreeNode();
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
	virtual void SetParent(QSharedPointer<iAImageTreeNode > parent);
	virtual QSharedPointer<iAImageTreeNode > GetParent() const;
	virtual QSharedPointer<iAImageTreeNode > GetChild(int idx) const = 0;
	virtual double GetAttribute(int) const = 0;
	virtual void GetMinMax(int chartID, double & min, double & max,
		iAChartAttributeMapper const & chartAttrMap) const = 0;
	virtual ClusterDistanceType GetDistance() const = 0;
	virtual Attitude GetAttitude() const;
	virtual void SetAttitude(Attitude att);
	virtual Attitude ParentAttitude() const;
	virtual LabelPixelHistPtr UpdateLabelDistribution() const = 0;
	virtual CombinedProbPtr UpdateProbabilities() const = 0;
	virtual void GetSelection(QVector<QSharedPointer<iASingleResult> > & result) const = 0;
private:
	QSharedPointer<iAImageTreeNode > m_parent;
	Attitude m_attitude;
};

void FindNode(iAImageTreeNode const * searched, QList<QSharedPointer<iAImageTreeNode> > & path, QSharedPointer<iAImageTreeNode> curCluster, bool & found);
QSharedPointer<iAImageTreeNode> GetSibling(QSharedPointer<iAImageTreeNode> node);


template<typename VisitorFn>
void VisitNodes(iAImageTreeNode const * node, VisitorFn visitor)
{
	visitor(node);
	for (int i = 0; i<node->GetChildCount(); ++i)
	{
		VisitNodes(node->GetChild(i).data(), visitor);
	}
}
