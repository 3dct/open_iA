#include "iAMinkowskiDistance.h"

//Debug
#include "iALog.h"

#include <cmath>

iAMinkowskiDistance::iAMinkowskiDistance() : iASimilarityDistance(), m_p(1.0)
{
}

void iAMinkowskiDistance::setOrder(int p)
{
	m_p = p;
}

void iAMinkowskiDistance::calculateSimilarityDistance(
	csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix)
{
	int amountRows = csvDataType::getRows(distanceMatrix);
	int amountCols = csvDataType::getColumns(distanceMatrix);

	if (csvDataType::getRows(dataMatrix) != amountRows || csvDataType::getRows(dataMatrix) != amountCols)
	{
		LOG(lvlDebug, "Minkowski Distance Calculation: Invalid distance matrix dimension.\n");
		return;
	}


	double temp;
	double p = (double)m_p;

	for (int r = 0; r < amountRows - 1; r++)
	{
		for (int c = r + 1; c < amountCols; c++)
		{
			temp = 0;
			for (int c2 = 0; c2 < csvDataType::getColumns(dataMatrix); c2++)
			{
				temp += pow(dataMatrix->at(r).at(c2) - dataMatrix->at(c).at(c2), p);
			}
			distanceMatrix->at(r).at(c) = pow(temp, (1 / p));
		}
	}

	for (int r = 1; r < amountRows; r++)
	{
		for (int c = 0; c < r; c++)
		{
			distanceMatrix->at(r).at(c) = distanceMatrix->at(c).at(r);
		}
	}

}