// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iASimilarityDistance.h"

#include "iACsvDataStorage.h"

class iAMinkowskiDistance : public iASimilarityDistance
{
public:
	iAMinkowskiDistance();

	void calculateSimilarityDistance(csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix) override;

	//set the order p of the metric
	//p = 1 corresponds to the Manhattan distance, p = 2 corresponds to the Euclidean distance
	// p has to be > 1, otherwise it the Minkowski Distance is not a metric, since it violates the triangle inequality
	void setOrder(int p);

private:

	int m_p;
};
