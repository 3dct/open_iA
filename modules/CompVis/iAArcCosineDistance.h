#pragma once

#include "iAProximityDistance.h"

class iAArcCosineDistance : public iAProximityDistance
{
   public:
	iAArcCosineDistance(std::vector<double>* weights, csvDataType::ArrayType* data, int amountOfCharas, int amountOfElems);
	virtual csvDataType::ArrayType* calculateProximityDistance();

	private:
	double calculateCounter(std::vector<double> e1, std::vector<double> e2);
	double calculateDenominator(std::vector<double> e1, std::vector<double> e2);

};
