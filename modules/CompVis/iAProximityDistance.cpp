#include "iAProximityDistance.h"

iAProximityDistance::iAProximityDistance(
	std::vector<double>* weights, csvDataType::ArrayType* data, int amountOfCharas, int amountOfElems) 
	: m_weights(weights),
	m_matrix(data),
	m_amountOfCharas(amountOfCharas),
	m_amountOfElems(amountOfElems)
{
}