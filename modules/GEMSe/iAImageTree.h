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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IA_IMAGE_TREE_H
#define IA_IMAGE_TREE_H

#include "iAGEMSeConstants.h"
#include "iAImageTypes.h"

#include <itkImage.h>
#include <itkSmartPointer.h>

#include <QSharedPointer>
#include <QVector>


const int CHILD_NODE_NUMBER = 2;

class iAAttributeFilter;
class iASingleResult;
class iAImageClusterLeaf;

typedef int ClusterIDType;
typedef float ClusterDistanceType;

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

class iAImageClusterNode
{
public:
	enum Attitude {
		NoPreference,
		Liked,
		Hated
	};
	iAImageClusterNode();
	virtual int GetChildCount() const =0;
	virtual int GetClusterSize() const =0;
	virtual int GetFilteredSize() const =0;
	virtual void UpdateFilter(iAAttributeFilter const & filter) =0;
	virtual bool IsLeaf() const =0;
	//! median image for this cluster:
	virtual ClusterImageType const GetRepresentativeImage(int type) const =0;
	virtual void DiscardDetails() =0;
	virtual ClusterIDType GetID() const =0;
	virtual void GetExampleImages(QVector<iAImageClusterLeaf *> & result, int amount) =0;
	virtual void SetParent(QSharedPointer<iAImageClusterNode > parent);
	virtual QSharedPointer<iAImageClusterNode > GetParent() const;
	virtual QSharedPointer<iAImageClusterNode > GetChild(int idx) const =0;
	virtual double GetAttribute(AttributeID) const =0;
	virtual void GetMinMax(AttributeID attribID, double & min, double & max) const = 0;
	virtual ClusterDistanceType GetDistance() const =0;
	virtual Attitude GetAttitude() const;
	virtual void SetAttitude(Attitude att);
	virtual Attitude ParentAttitude() const;
	virtual LabelPixelHistPtr UpdateLabelDistribution() const =0;
	virtual CombinedProbPtr UpdateProbabilities() const = 0;
private:
	QSharedPointer<iAImageClusterNode > m_parent;
	Attitude m_attitude;
};


//! internal (i.e. non-leaf) tree node.
//! currently assumes that tree is not modified after creation!
class iAImageClusterInternal: public iAImageClusterNode
{
public:
	iAImageClusterInternal(
		QSharedPointer<iAImageClusterNode > a,
		QSharedPointer<iAImageClusterNode > b,
		LabelPixelType differenceMarkerValue,
		QString const & cachePath,
		ClusterIDType id,
		ClusterDistanceType distance);
	virtual int GetChildCount() const;
	virtual int GetClusterSize() const;
	virtual int GetFilteredSize() const;
	virtual void UpdateFilter(iAAttributeFilter const & filter);
	virtual bool IsLeaf() const { return false; }
	virtual ClusterImageType const GetRepresentativeImage(int type) const;
	virtual void DiscardDetails();
	virtual void DiscardFilterData();
	virtual ClusterIDType GetID() const;
	virtual void GetExampleImages(QVector<iAImageClusterLeaf *> & result, int amount);
	virtual QSharedPointer<iAImageClusterNode > GetChild(int idx) const;
	virtual double GetAttribute(AttributeID) const;
	virtual void GetMinMax(AttributeID attribID, double & min, double & max) const;
	virtual ClusterDistanceType GetDistance() const;
	virtual LabelPixelHistPtr UpdateLabelDistribution() const;
	virtual CombinedProbPtr UpdateProbabilities() const;
private:
	void RecalculateFilteredRepresentative(int type) const;
	QString GetCachedFileName(int type) const;
	ClusterImageType CalculateRepresentative(int type) const;
	ClusterImageType CalculateFilteredRepresentative(int type) const;
	ClusterIDType m_ID;
	int m_clusterSize;
	int m_filteredSize;
	mutable bool m_filteredRepresentativeOutdated;
	LabelPixelType m_differenceMarkerValue; // TODO: find way to get rid of this variable
	mutable QVector<ClusterImageType> m_representative;
	mutable QVector<ClusterImageType> m_filteredRepresentative;
	std::pair<QSharedPointer<iAImageClusterNode >, QSharedPointer<iAImageClusterNode > > m_children;
	ClusterDistanceType m_distance;
	QString m_cachePath;
	int m_labelCount;
};


class iAImageClusterLeaf: public iAImageClusterNode
{
public:
	iAImageClusterLeaf(QSharedPointer<iASingleResult> img, int labelCount);
	virtual int GetChildCount() const;
	virtual int GetClusterSize() const;
	virtual int GetFilteredSize() const;
	virtual void UpdateFilter(iAAttributeFilter const & filter);
	virtual ClusterImageType const GetRepresentativeImage(int type) const;
	virtual void DiscardDetails();
	virtual ClusterImageType const GetDetailImage() const;
	virtual ClusterIDType GetID() const;
	virtual bool IsLeaf() const { return true; }
	virtual void GetExampleImages(QVector<iAImageClusterLeaf *> & result, int amount);
	virtual QSharedPointer<iAImageClusterNode > GetChild(int idx) const;
	ClusterImageType const GetLargeImage() const;
	virtual double GetAttribute(AttributeID) const;
	virtual void GetMinMax(AttributeID attribID, double & min, double & max) const;
	virtual ClusterDistanceType GetDistance() const;
	void SetAttribute(int id, double value);
	virtual LabelPixelHistPtr UpdateLabelDistribution() const;
	virtual CombinedProbPtr UpdateProbabilities() const;
private:
	bool m_filtered;
	int m_labelCount;
	QSharedPointer<iASingleResult> m_singleResult;
};


class iAAttributeFilter;
class QTextStream;

class iAImageTree
{
public:
	static QSharedPointer<iAImageTree> Create(QString const & fileName, QVector<QSharedPointer<iASingleResult> > const & sampleResults, int labelCount);
	iAImageTree(QSharedPointer<iAImageClusterNode >, int labelCount);
	QSharedPointer<iAImageClusterNode > m_root;
	bool Store(QString const & fileName) const;
	int GetLabelCount() const;
private:
	static void WriteNode(QTextStream & out, QSharedPointer<iAImageClusterNode >, int level);
	static QSharedPointer<iAImageClusterNode> ReadNode(
		QTextStream & in,
		QVector<QSharedPointer<iASingleResult> > const & sampleResults,
		int labelCount,
		QString const & outputDirectory,
		int & lastClusterID);
	int m_labelCount;
};

void GetClusterMinMax(iAImageClusterNode const * node, AttributeID attribID, double & min, double & max);

#endif // IA_IMAGE_TREE_H