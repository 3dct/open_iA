// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iASimilarityDistance.h"

//CompVis
#include "iACsvDataStorage.h"

class iAEuclideanDistance : public iASimilarityDistance
{
public:
	iAEuclideanDistance();
	void calculateSimilarityDistance(csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix) override;
};
