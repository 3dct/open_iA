#include "iAArcCosineDistance.h"

#include <math.h>

//Debug
#include "iAConsole.h"

iAArcCosineDistance::iAArcCosineDistance(
	std::vector<double>* weights, csvDataType::ArrayType* data, int amountOfCharas, int amountOfElems) :
	iAProximityDistance(weights, data, amountOfCharas, amountOfElems)
{
}

csvDataType::ArrayType* iAArcCosineDistance::calculateProximityDistance()
{
	m_distanceMatrix = new csvDataType::ArrayType();
	csvDataType::initialize(m_amountOfElems, m_amountOfElems, m_distanceMatrix);

	//DEBUG
	/*DEBUG_LOG("");
	DEBUG_LOG("m_matrix");
	csvDataType::debugArrayType(m_matrix);
	DEBUG_LOG("");*/

	/*DEBUG_LOG("amountOfCharas = " + QString::number(m_amountOfCharas));
	DEBUG_LOG("amountOfElems = " + QString::number(m_amountOfElems));*/

	for (int pass1 = 0; pass1 < m_amountOfElems; pass1++)
	{
		for (int pass2 = 0; pass2 < m_amountOfElems; pass2++)
		{
	
			/*DEBUG_LOG("");*/

			double counter = calculateCounter(m_matrix->at(pass1), m_matrix->at(pass2));
			double denominator = calculateDenominator(m_matrix->at(pass1), m_matrix->at(pass2));

			//DEBUG_LOG("counter = " + QString::number(counter));
			//DEBUG_LOG("denominator = " + QString::number(denominator));
			//

			if(std::abs(counter - denominator) < 1.0e-07)
			{ // catch arithemtic error
				m_distanceMatrix->at(pass1).at(pass2) = acos(1);

			}else
			{
				double division = counter / denominator;
				m_distanceMatrix->at(pass1).at(pass2) = acos(division);

			}

		/*	DEBUG_LOG("m_distanceMatrix->at(pass1).at(pass2) = " + QString::number(m_distanceMatrix->at(pass1).at(pass2)));
			DEBUG_LOG("");*/
		}
	}

	//DEBUG
	/*DEBUG_LOG("");
	DEBUG_LOG("m_distanceMatrix");
	csvDataType::debugArrayType(m_distanceMatrix);
	DEBUG_LOG("");*/

	return m_distanceMatrix;
}

double iAArcCosineDistance::calculateCounter(std::vector<double> e1, std::vector<double> e2)
{
	double result = 0.0;
	
	for (int col = 0; col < m_amountOfCharas; col++)
	{
	/*	DEBUG_LOG("m_weights->at(col) = " + QString::number(m_weights->at(col)));*/

		double w = std::pow(m_weights->at(col), 2);

	/*	DEBUG_LOG("w = " + QString::number(w));
		

		DEBUG_LOG("e1.at( " + QString::number(col) + " ) = " + QString::number(e1.at(col)));
		DEBUG_LOG("e2.at( " + QString::number(col) + " ) = " + QString::number(e2.at(col)));

		DEBUG_LOG("w * e1.at(col) * e2.at(col) = " + QString::number(w * e1.at(col) * e2.at(col)));*/

		result += (w * e1.at(col) * e2.at(col));
	}
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
	return result;
}