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
#include "iAImageClusterer.h"

#include "iAConsole.h"
#include "iAImageTree.h"
#include "iAMathUtility.h"
#include "iARepresentative.h"

#include "iAImageComparisonMetrics.h"

#include <itkLabelOverlapMeasuresImageFilter.h>


#include <QMap>

#include <utility>

iAImageClusterer::iAImageClusterer(int labelCount, QString const & outputDirectory):
	m_labelCount(labelCount),
	m_aborted(false),
	m_remainingNodes(0),
	m_currImage(-1),
	m_imageDistCalcDuration(0.0),
	m_outputDirectory(outputDirectory)
{
}

#include "iASingleResult.h"

void iAImageClusterer::AddImage(QSharedPointer<iASingleResult> singleResult)
{
	m_images.push_back(QSharedPointer<iAImageClusterNode>(new iAImageClusterLeaf(singleResult, m_labelCount)));
}


QSharedPointer<iAImageTree > iAImageClusterer::GetResult()
{
	return m_tree;
}

#include "iAImageComparisonMetrics.h"

double CalcDistance(ClusterImageType img1, ClusterImageType img2)
{
	assert (img1);
	assert (img2);
	double meanOverlap = 0;
	try
	{
		/*
		// TODO: find out why this is not working (sometimes)
		typedef itk::LabelOverlapMeasuresImageFilter<LabelImageType > FilterType;
		FilterType::Pointer filter = FilterType::New();
		filter->SetSourceImage(img1);
		filter->SetTargetImage(img2);
		filter->Update();
		meanOverlap = filter->GetMeanOverlap();
		*/
		iAImageComparisonResult r = CompareImages(img1, img2);
		meanOverlap = r.equalPixelRate;
	}
	catch (itk::ExceptionObject & e)
	{
		DEBUG_LOG(QString("itk Exception: %1\n").arg(e.GetDescription()).toStdString());
		return 0.0;
	}
	if (isNaN(meanOverlap))
	{
		//DebugOut() << "ERROR: CalcDistance -> NAN!" << std::endl;
		return 1.0;
	}
	return 1-meanOverlap;
}

int triangularNumber(int num)
{
	return ((num-1)*num)/2;
}

// TODO: use better data structure?
// or use SLINT or other hierarch. clustering
template <typename ValueType>
class DiagonalMatrix
{
public:
	DiagonalMatrix(int side):
		m_storageSize(triangularNumber(side)),
		m_side(side),
		m_values(new ValueType[m_storageSize])
	{
		for (int i=0; i<m_storageSize; ++i)
		{
			m_values[i] = std::numeric_limits<ValueType>::max();
		}
	}
	bool checkIndex(int x, int y)
	{
		if (x < 0 || x >= m_side || y < 0 || y > m_side)
		{
			DEBUG_LOG(QString("Clusterer::DiagonalMatrix: GetValue - index %1, %2  outside of valid range 0..%3\n")
				.arg(x)
				.arg(y)
				.arg(m_side - 1) );
			return false;
		}
		if (x == y)
		{
			DEBUG_LOG(QString("Clusterer::DiagonalMatrix: SetValue - Invalid access: diagonal! x==y==\n")
				.arg(x));
			return false;
		}
		return true;
	}
	void SetValue(int x, int y, ValueType value)
	{
		if (!checkIndex(x, y))
		{
			return;
		}
		assert(x >= 0 && y >= 0 && x < m_side && y < m_side);
		m_values[translate(x, y)] = value;
	}

	ValueType GetValue(int x, int y)
	{
		if (!checkIndex(x, y))
		{
			return 0;
		}
		assert(x >= 0 && y >= 0 && x < m_side && y < m_side);
		return m_values[translate(x, y)];
	}

	void Remove(int idx)
	{
		if (idx < 0 || idx >= m_side)
		{
			DEBUG_LOG(QString("Clusterer::DiagonalMatrix: Remove - index %1 outside of valid range 0..%2!\n")
				.arg(idx)
				.arg(m_side - 1));
			return;
		}
		assert(idx >= 0 && idx < m_side);
		//bool anyNotMax = false;
		for (int i=0; i<m_side; ++i)
		{
			if (i == idx) continue;
			int diagIdx = translate(idx, i);
			//anyNotMax |= (m_values[diagIdx] != std::numeric_limits<ValueType>::max());
			m_values[diagIdx] = std::numeric_limits<ValueType>::max();
		}
		/*
		if (!anyNotMax)
		{
			DebugOut() << "Clusterer::DiagonalMatrix:: Remove - index "<<idx<<" seems to have been removed already before!" << std::endl;
		}
		*/
	}
	std::pair<int, int> GetMinimum()
	{
		std::pair<int, int> minIdx;
		ValueType min1 = std::numeric_limits<ValueType>::max();
		for (int x=0; x<m_side-1; ++x)
		{
			for (int y=x+1; y<m_side; ++y)
			{
				ValueType dist = GetValue(x, y);
				if (dist < min1)
				{
					min1 = dist;
					minIdx = std::make_pair(x, y);
				}
			}
		}
		if (minIdx.first == minIdx.second)
		{
			DEBUG_LOG(QString("Clusterer::DiagonalMatrix: GetMinimum - DiagonalMatrix seems to be empty already (m_side=%1)!\n")
				.arg(m_side));
		}
//		assert(minIdx.first != minIdx.second);
		return minIdx;
	}

	void print()
	{
		for (int i=0; i<m_storageSize; ++i)
		{
			if (m_values[i] == std::numeric_limits<ValueType>::max())
			{
				std::cout << "* ";
			}
			else
			{
				std::cout << m_values[i] << " ";
			}
		}
		std::cout << std::endl;
	}
	void prettyPrint()
	{
		for (int y=0; y<m_side; ++y)
		{
			for (int x=0; x<m_side; ++x)
			{
				if (x == y)
				{
					std::cout << "- ";
				}
				else if (m_values[translate(x, y)] == std::numeric_limits<ValueType>::max())
				{
					std::cout << "* ";
				}
				else
				{
					std::cout << m_values[translate(x, y)] << " ";
				}
			}
			std::cout << std::endl;
		}
	}
private:
	DiagonalMatrix(const DiagonalMatrix& that) = delete;
	// source: http://www.codeguru.com/cpp/cpp/algorithms/general/article.php/c11211/TIP-Half-Size-Triangular-Matrix.htm
	int translate(int x, int y)
	{
		assert(x != y);    //You can add this in if you like
		if (y<x)
			std::swap(x, y);

		int result = x*(m_side-1) - (x-1)*((x-1) + 1)/2 + y - x - 1;
		return result;
	}
private:
	int m_side;
	int m_storageSize;
	ValueType* m_values;
};

namespace {
	const int FullProgress = 100;
	const int SplitFactorDistanceCalc = 50;

	long sumUpTo(int n)
	{
		return static_cast<long>(n)*(n+1) / 2;
	}
	long sumUpToDiff(int n1, int n2)
	{
		return sumUpTo(n1) - sumUpTo(n1-n2);
	}
}


bool IsEmpty(DiagonalMatrix<float> & distances, int idx, int cnt)
{
	bool result = true;
	DEBUG_LOG(QString("%1 -> ").arg(idx));
	for (int i=0; i<cnt; ++i)
	{
		if (i == idx) continue;
		if (distances.GetValue(i, idx) != std::numeric_limits<float>::max())
		{
			DEBUG_LOG(QString("%1:%2 ")
				.arg(i)
				.arg(distances.GetValue(i, idx)));
			result = false;
			return false;
		}
	}
	if (result)
	{
		DEBUG_LOG(QString("%1 is empty!").arg(idx));
	}
	DEBUG_LOG("\n");
	return true;
}

void iAImageClusterer::run()
{
	m_remainingNodes = m_images.size();
	m_perfTimer.start();
	emit Status("Calculating distances for all image pairs");
	DiagonalMatrix<float> distances((m_images.size()*2)-1);
	for (m_currImage=0; m_currImage<m_images.size() && !m_aborted; ++m_currImage)
	{
		emit Status(QString("Calculating distances for image pairs, image ")+QString::number(m_currImage) +" of " +QString::number(m_images.size()) );
		// assuming here that the metric is symmetric
		for (int j=m_currImage+1; j<m_images.size() && !m_aborted; ++j)
		{
			ClusterImageType img1 = m_images[m_currImage]->GetRepresentativeImage(iARepresentativeType::Difference).GetPointer();
			ClusterImageType img2 = m_images[j]->GetRepresentativeImage(iARepresentativeType::Difference).GetPointer();
			float distance = 1.0;
			if (!img1 || !img2)
			{
				DEBUG_LOG("Invalid (non-label) representative difference image");
				distance = 1.0;
			}
			else
			{
				distance = CalcDistance(img1, img2);
			}
			if (isNaN(distance))
			{
				DEBUG_LOG(QString("ERROR: %1, %2 -> NAN!")
					.arg(m_currImage)
					.arg(j));
				distance = 1.0;
			}
			m_images[j]->DiscardDetails();
			distances.SetValue(m_currImage, j, distance);
		}
		m_images[m_currImage]->DiscardDetails();
		emit Progress(SplitFactorDistanceCalc * 
			(static_cast<double>(sumUpToDiff(m_images.size(), m_currImage))/ sumUpTo(m_images.size())) );
	}
	//distances.prettyPrint();
	m_imageDistCalcDuration = m_perfTimer.elapsed();
	m_perfTimer.start();
	emit Status("Hierarchical clustering.");
	assert(m_images.size() > 0);
	QSharedPointer<iAImageClusterNode> lastNode = m_images[0];
	int clusterID = m_remainingNodes;
	while (m_remainingNodes > 1 && !m_aborted) // we need to do n-1 merges
	{
		emit Status(QString("Hierarchical clustering (")+QString::number(m_remainingNodes)+" remaining nodes)");
		// get minimum distance pair:
		std::pair<int, int> idx = distances.GetMinimum();
		assert (idx.first != idx.second);

		if (idx.first == idx.second)
		{
			if (idx.first == 0)
			{
				DEBUG_LOG(QString("Premature exit with %1 nodes remaining: ").arg(m_remainingNodes));
				for (int i=0; i<m_images.size(); ++i)
				{
					if (!m_images[i].isNull())
					{
						IsEmpty(distances, i, m_images.size());
					}
				}
				DEBUG_LOG("\n");
				break;
			}
			DEBUG_LOG(QString("Clustering: Two times %1 is not a valid pair!").arg(idx.first));
			distances.Remove(idx.first);
			continue;
		}

		if (!m_images[idx.first] || !m_images[idx.second])
		{
			DEBUG_LOG(QString("Clustering: One or both of images to cluster already clustered (%1, %2\n")
				.arg(m_images[idx.first] ? "first set" : "!first NOT set!")
				.arg(m_images[idx.second] ? "second set" : "!second NOT set!\n"));
			DEBUG_LOG(QString("Premature exit with %1 nodes remaining: ").arg(m_remainingNodes));
			for (int i=0; i<m_images.size(); ++i)
			{
				if (!m_images[i].isNull())
				{
					IsEmpty(distances, i, m_images.size());
				}
			}
			DEBUG_LOG("\n");
			break;
		}
		// create merged node:
		lastNode = QSharedPointer<iAImageClusterInternal>(new iAImageClusterInternal(
			m_images[idx.first], m_images[idx.second],
			m_labelCount,
			m_outputDirectory,
			clusterID++,
			distances.GetValue(idx.first, idx.second)
		));
		m_images[idx.first ]->SetParent(lastNode);
		m_images[idx.second]->SetParent(lastNode);
		m_images[idx.first ]->DiscardDetails();
		m_images[idx.second]->DiscardDetails();
		
		m_images.push_back(lastNode);
		
		// recalculate distances:
		int newItemIdx = m_images.size()-1;
		//DebugOut() << "Clustered " << idx.first << ", " << idx.second << " to " << newItemIdx << std::endl;

		for (int i=0; i<m_images.size()-1 && !m_aborted; ++i) // last one is the just inserted one, skip that
		{
			if (i == idx.first || i == idx.second || !m_images[i]) // skip deleted ones
				continue;
			assert(m_images[idx.first]);
			assert(m_images[idx.second]);
			float distance = std::max(		// maximum-linkage
				distances.GetValue(i, idx.first),
				distances.GetValue(i, idx.second)
			);
			// index translation should take care that everything gets into the right place
			distances.SetValue(i, newItemIdx, distance);
		}

		// remove from matrix & list:

		distances.Remove(idx.first);
		m_images[idx.first] = QSharedPointer<iAImageClusterNode>();

		distances.Remove(idx.second);
		m_images[idx.second] = QSharedPointer<iAImageClusterNode>();

		--m_remainingNodes;
		emit Progress((SplitFactorDistanceCalc + (FullProgress-SplitFactorDistanceCalc)*(m_images.size()-m_remainingNodes)) / m_images.size());
	}
	if (!m_aborted)
	{
		m_tree = QSharedPointer<iAImageTree>(new iAImageTree(lastNode, m_labelCount));
	}
}



void iAImageClusterer::Abort()
{
	m_aborted = true;
}

bool iAImageClusterer::IsAborted()
{
	return m_aborted;
}


double iAImageClusterer::elapsed() const
{
	return m_imageDistCalcDuration + m_perfTimer.elapsed();
}

double iAImageClusterer::estimatedTimeRemaining() const
{
	// estimated time given until current step (image distance calc / clustering) finished, not whole operation
	if (m_imageDistCalcDuration == 0.0)
	{
		return (m_perfTimer.elapsed() / sumUpToDiff(m_images.size(), m_currImage)) // average duration of one image comparison
			* sumUpTo(m_images.size()-m_currImage); // number of image comparisons still to do
	}
	else
	{
		return (m_perfTimer.elapsed() / (m_images.size()-m_remainingNodes))  // average duration of one cycle
			* m_remainingNodes;
	}
}
