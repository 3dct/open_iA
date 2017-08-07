/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAGraphWeights.h"

#include "iANormalizer.h"
#include "iAImageGraph.h"
#include "iAVectorArray.h"
#include "iAVectorDistance.h"

#include <algorithm>
#include <cassert>

iAGraphWeights::iAGraphWeights(iAEdgeIndexType edgeCount):
m_weights(edgeCount)
{}

iAEdgeWeightType iAGraphWeights::GetMaxWeight() const
{
	return *std::max_element(m_weights.begin(), m_weights.end());
}

iAEdgeWeightType iAGraphWeights::GetWeight(iAEdgeIndexType edgeIdx) const
{
	return m_weights[edgeIdx];
}

void iAGraphWeights::SetWeight(iAEdgeIndexType edgeIdx, iAEdgeWeightType weight)
{
	assert(edgeIdx < m_weights.size());
	m_weights[edgeIdx] = weight;
}

void iAGraphWeights::Normalize(QSharedPointer<iANormalizer> normalizeFunc)
{
	iAEdgeWeightType max = GetMaxWeight();
	normalizeFunc->SetMaxValue(max);
	for (int i=0; i<m_weights.size(); ++i)
	{
					// 1-x - because we need "resistance" for RW, not "conductance"
		m_weights[i] = 1 - normalizeFunc->Normalize(m_weights[i]);
	}
}

int iAGraphWeights::GetEdgeCount() const
{
	return m_weights.size();
}

QSharedPointer<iAGraphWeights> CalculateGraphWeights(
	iAImageGraph const & graph,
	iAVectorArray const & voxelData,
	iAVectorDistance const & distanceFunc)
{
	QSharedPointer<iAGraphWeights> result(new iAGraphWeights(graph.GetEdgeCount()));
	for (int i=0; i<graph.GetEdgeCount(); ++i)
	{
		iAEdgeType edge = graph.GetEdge(i);
		iADistanceType dist = distanceFunc.GetDistance(voxelData.get(edge.first), voxelData.get(edge.second));
		result->SetWeight(i, dist);
	}
	return result;
}


QSharedPointer<iAGraphWeights const> CombineGraphWeights(
	QVector<QSharedPointer<iAGraphWeights>> const & graphWeights,
	QVector<double> const & weight)
{
	assert(graphWeights.size() > 0);
	assert(graphWeights.size() == weight.size());
	int edgeCount = graphWeights[0]->GetEdgeCount();
	QSharedPointer<iAGraphWeights> result(new iAGraphWeights(edgeCount));
	for (int edgeIdx=0; edgeIdx<edgeCount; ++edgeIdx)
	{
		double combinedWeight = 0;
		for (int channelIdx=0; channelIdx<graphWeights.size(); ++channelIdx)
		{
			combinedWeight += weight[channelIdx] * graphWeights[channelIdx]->GetWeight(edgeIdx);
		}
		combinedWeight += iAVectorDistance::EPSILON;
		result->SetWeight(edgeIdx, combinedWeight);
	}
	return result;
}
