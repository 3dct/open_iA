// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACsvDataStorage.h"

/*
This class represents the approximated distance matrix
*/
class iASimilarityDistance
{
   public:
	iASimilarityDistance();
	virtual void calculateSimilarityDistance(
		csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix) = 0;
};
