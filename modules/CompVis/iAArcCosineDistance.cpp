#include "iAArcCosineDistance.h"

#include <cmath>

//Debug
#include "iAConsole.h"

iAArcCosineDistance::iAArcCosineDistance(
	std::vector<double>* weights, csvDataType::ArrayType* data, int amountOfCharas, int amountOfElems) :
	iAProximityDistance(weights, data, amountOfCharas, amountOfElems)
{
	calculateProximityDistance();
}

csvDataType::ArrayType* iAArcCosineDistance::calculateProximityDistance()
{
	std::vector<double> vec(m_amountOfCharas);
	m_distanceMatrix = new csvDataType::ArrayType(m_amountOfElems, vec);

	for (int pass1 = 0; pass1 < m_amountOfElems; pass1++)
	{
		for (int pass2 = 0; pass2 < m_amountOfElems; pass2++)
		{
			double counter = calculateCounter(m_matrix->at(pass1), m_matrix->at(pass2));
			double denominator = calculateDenominator(m_matrix->at(pass1), m_matrix->at(pass2));

			m_distanceMatrix->at(pass1).at(pass2) = acos(counter / denominator);
		}
	}

	//DEBUG
	for (int col = 0; col < 1; col++)
	{
		for (int row = 0; row < m_amountOfCharas; row++)
		{
			DEBUG_LOG(QString::number(m_distanceMatrix->at(col).at(row)));
		}
	}

	return m_distanceMatrix;
}

double iAArcCosineDistance::calculateCounter(std::vector<double> e1, std::vector<double> e2)
{
	double result = 0.0;
	
	for (int row = 0; row < m_amountOfCharas; row++)
	{
		double w = std::pow(m_weights->at(row), 2);
		result += (w * e1.at(row) * e2.at(row));
	}

	//DEBUG_LOG("Counter: " + QString::number(result));
	return result;
}

double iAArcCosineDistance::calculateDenominator(std::vector<double> e1, std::vector<double> e2)
{
	double result1 = 0.0;
	double result2 = 0.0;
	double result = 0.0;

	for (int row = 0; row < m_amountOfCharas; row++)
	{
		double w = std::pow(m_weights->at(row), 2);
		result1 += (w * std::pow(e1.at(row), 2));
		result2 += (w * std::pow(e2.at(row), 2));
	}

	result = std::sqrt(result1 * result2);

	//DEBUG_LOG("Denominator: " + QString::number(result));
	return result;
}