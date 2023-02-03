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
