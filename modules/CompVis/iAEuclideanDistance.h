#pragma once

#include "iASimilarityDistance.h"

//CompVis
#include "iACsvDataStorage.h"

class iAEuclideanDistance : public iASimilarityDistance
{
   public:
	iAEuclideanDistance();
	void calculateSimilarityDistance(csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix);
};