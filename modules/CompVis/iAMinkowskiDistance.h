#pragma once
#include "iASimilarityDistance.h"

#include "iACsvDataStorage.h"

class iAMinkowskiDistance : public iASimilarityDistance
{
   public:
	iAMinkowskiDistance();
	void calculateSimilarityDistance(
		csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix);
};
