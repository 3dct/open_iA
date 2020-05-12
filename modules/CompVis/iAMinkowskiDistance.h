#pragma once
#include "iASimilarityDistance.h"

#include "iACsvDataStorage.h";

class iAMinkowskiDistance : public iASimilarityDistance
{
   public:
	iAMinkowskiDistance();
	csvDataType::ArrayType* calculateSimilarityDistance(
		csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix);
};