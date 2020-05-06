#pragma once

#include "iACsvDataStorage.h"
#include <vector>

class iAProximityDistance
{
   public:
	iAProximityDistance(std::vector<double>* weights, csvDataType::ArrayType* data, int amountOfCharas, int amountOfElems);

	virtual csvDataType::ArrayType* calculateProximityDistance() = 0;

   protected:
	//user defined weights in the range of [0,1]
	std::vector<double>* m_weights;
	//matrix for which the calculation will be done
	csvDataType::ArrayType* m_matrix;
	//amount of characertistics
	int m_amountOfCharas;
	//amount of objects
	int m_amountOfElems;
	//resulting matrix with the pairwise distances
	csvDataType::ArrayType* m_distanceMatrix;
};