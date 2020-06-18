#include "iAEuclideanDistance.h"

iAEuclideanDistance::iAEuclideanDistance() : iASimilarityDistance()
{
}

csvDataType::ArrayType* iAEuclideanDistance::calculateSimilarityDistance(
	csvDataType::ArrayType* dataMatrix, csvDataType::ArrayType* distanceMatrix)
{

	int amountRows = csvDataType::getRows(distanceMatrix);
	int amountCols = csvDataType::getColumns(distanceMatrix);

	if (csvDataType::getRows(dataMatrix) != amountRows || csvDataType::getRows(dataMatrix) != amountCols)
	{
		DEBUG_LOG("Euclidean Distance Calculation: Invalid distance matrix dimension.\n");
		return distanceMatrix;
	}

	csvDataType::ArrayType* resultMatrix = distanceMatrix;

	double temp;

	for (int r = 0; r < amountRows - 1; r++)
	{
		for (int c = r + 1; c < amountCols; c++)
		{
			temp = 0;
			for (int c2 = 0; c2 < csvDataType::getColumns(dataMatrix); c2++)
			{
				temp += pow(dataMatrix->at(r).at(c2) - dataMatrix->at(c).at(c2), 2);
			}
			resultMatrix->at(r).at(c) = std::sqrt(temp);
		}
	}

	for (int r = 1; r < amountRows; r++)
	{
		for (int c = 0; c < r; c++)
		{
			resultMatrix->at(r).at(c) = resultMatrix->at(c).at(r);
		}
	}

	//DEBUG
	//DEBUG_LOG("Euclidean Distance:");
	//csvDataType::debugArrayType(resultMatrix);

	return resultMatrix;
}