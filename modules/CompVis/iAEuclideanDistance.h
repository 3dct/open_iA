#pragma once

#include "iASimilarityDistance.h"

//CompVis
#include "iACsvDataStorage.h"

class iAEuclideanDistance : public iASimilarityDistance
{
   public:
	iAEuclideanDistance();
	csvDataType::ArrayType* calculateSimilarityDistance(csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix);
};