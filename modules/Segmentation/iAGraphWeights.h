// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageGraphTypes.h"

#include <QVector>

#include <memory>

class iANormalizer;
class iAImageGraph;
class iAVectorDistance;
class iAVectorArray;

class iAGraphWeights
{
public:
	iAGraphWeights(iAEdgeIndexType edgeCount);
	void Normalize(std::shared_ptr<iANormalizer> normalizeFunc);
	iAEdgeWeightType GetMaxWeight() const;
	iAEdgeWeightType GetWeight(iAEdgeIndexType edgeIdx) const;
	void SetWeight(iAEdgeIndexType edgeIdx, iAEdgeWeightType weight);
	int GetEdgeCount() const;
private:
	QVector<iAEdgeWeightType> m_weights;
};

std::shared_ptr<iAGraphWeights> CalculateGraphWeights(
	iAImageGraph const & graph,
	iAVectorArray const & voxelData,
	iAVectorDistance const & distanceFunc
);

std::shared_ptr<iAGraphWeights const> CombineGraphWeights(
	QVector<std::shared_ptr<iAGraphWeights>> const & graphWeights,
	QVector<double> const & weight
);
