#pragma once

#include "iACsvDataStorage.h"

/*
This class represents the approximated distance matrix
*/
class iASimilarityDistance
{
   public:
	iASimilarityDistance();
	virtual csvDataType::ArrayType* calculateSimilarityDistance(
		csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix) = 0;
};