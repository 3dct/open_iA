#include "iAArcCosineDistance.h"

#include <math.h>

//Debug
#include "iALog.h"

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
	/*LOG(lvlDebug);
	LOG(lvlDebug,"m_matrix");
	csvDataType::debugArrayType(m_matrix);
	LOG(lvlDebug,"");*/

	/*LOG(lvlDebug,"amountOfCharas = " + QString::number(m_amountOfCharas));
	LOG(lvlDebug,"amountOfElems = " + QString::number(m_amountOfElems));*/

	for (int pass1 = 0; pass1 < m_amountOfElems; pass1++)
	{
		for (int pass2 = 0; pass2 < m_amountOfElems; pass2++)
		{
	
			/*LOG(lvlDebug,"");*/

			double counter = calculateCounter(m_matrix->at(pass1), m_matrix->at(pass2));
			double denominator = calculateDenominator(m_matrix->at(pass1), m_matrix->at(pass2));

			//LOG(lvlDebug,"counter = " + QString::number(counter));
			//LOG(lvlDebug,"denominator = " + QString::number(denominator));
			//

			if(std::abs(counter - denominator) < 1.0e-07)
			{ // catch arithemtic error
				m_distanceMatrix->at(pass1).at(pass2) = acos(1);

			}else
			{
				double division = counter / denominator;
				m_distanceMatrix->at(pass1).at(pass2) = acos(division);

			}

		/*	LOG(lvlDebug,"m_distanceMatrix->at(pass1).at(pass2) = " + QString::number(m_distanceMatrix->at(pass1).at(pass2)));
			LOG(lvlDebug,"");*/
		}
	}

	//DEBUG
	/*LOG(lvlDebug,"");
	LOG(lvlDebug,"m_distanceMatrix");
	csvDataType::debugArrayType(m_distanceMatrix);
	LOG(lvlDebug,"");*/

	return m_distanceMatrix;
}

double iAArcCosineDistance::calculateCounter(std::vector<double> e1, std::vector<double> e2)
{
	double result = 0.0;
	
	for (int col = 0; col < m_amountOfCharas; col++)
	{
	/*	LOG(lvlDebug,"m_weights->at(col) = " + QString::number(m_weights->at(col)));*/

		double w = std::pow(m_weights->at(col), 2);

	/*	LOG(lvlDebug,"w = " + QString::number(w));
		

		LOG(lvlDebug,"e1.at( " + QString::number(col) + " ) = " + QString::number(e1.at(col)));
		LOG(lvlDebug,"e2.at( " + QString::number(col) + " ) = " + QString::number(e2.at(col)));

		LOG(lvlDebug,"w * e1.at(col) * e2.at(col) = " + QString::number(w * e1.at(col) * e2.at(col)));*/

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