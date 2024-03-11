// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	assert(edgeIdx < static_cast<unsigned int>(m_weights.size()));
	m_weights[edgeIdx] = weight;
}

void iAGraphWeights::Normalize(std::shared_ptr<iANormalizer> normalizeFunc)
{
	iAEdgeWeightType max = GetMaxWeight();
	normalizeFunc->SetMaxValue(max);
	for (int i=0; i<m_weights.size(); ++i)
	{
					// 1-x - because we need "resistance" for RW, not "conductance"
		m_weights[i] = 1 - normalizeFunc->Normalize(m_weights[i]);
	}
}

qsizetype iAGraphWeights::GetEdgeCount() const
{
	return m_weights.size();
}

std::shared_ptr<iAGraphWeights> CalculateGraphWeights(
	iAImageGraph const & graph,
	iAVectorArray const & voxelData,
	iAVectorDistance const & distanceFunc)
{
	auto result = std::make_shared<iAGraphWeights>(graph.edgeCount());
	for (iAEdgeIndexType i=0; i<graph.edgeCount(); ++i)
	{
		iAEdgeType edge = graph.edge(i);
		iADistanceType dist = distanceFunc.GetDistance(voxelData.get(edge.first), voxelData.get(edge.second));
		result->SetWeight(i, dist);
	}
	return result;
}


std::shared_ptr<iAGraphWeights const> CombineGraphWeights(
	QVector<std::shared_ptr<iAGraphWeights>> const & graphWeights,
	QVector<double> const & weight)
{
	assert(graphWeights.size() > 0);
	assert(graphWeights.size() == weight.size());
	auto edgeCount = graphWeights[0]->GetEdgeCount();
	std::shared_ptr<iAGraphWeights> result(new iAGraphWeights(edgeCount));
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
