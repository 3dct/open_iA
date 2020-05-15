#include "iAArcCosineDistance.h"

#include <math.h>

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
	m_distanceMatrix = csvDataType::initialize(m_amountOfElems, m_amountOfElems);

	for (int pass1 = 0; pass1 < m_amountOfElems; pass1++)
	{
		for (int pass2 = 0; pass2 < m_amountOfElems; pass2++)
		{
			double counter = calculateCounter(m_matrix->at(pass1), m_matrix->at(pass2));
			double denominator = calculateDenominator(m_matrix->at(pass1), m_matrix->at(pass2));
			//DEBUG_LOG("counter: " + QString::number(counter));
			//DEBUG_LOG("denominator: " + QString::number(denominator));

			
			//DEBUG_LOG(QString("acos(counter / denominator): %1").arg(floor(counter / denominator)));
			//DEBUG_LOG("acos(counter / denominator): " + QString::number(acos(floor(counter / denominator))));
			
			m_distanceMatrix->at(pass1).at(pass2) = acos(floor(counter / denominator));
			//DEBUG_LOG(
			//	"m_distanceMatrix->at(pass1).at(pass2): " + QString::number(m_distanceMatrix->at(pass1).at(pass2)));
		}
	}

	//DEBUG
	//csvDataType::debugArrayType(m_distanceMatrix);

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