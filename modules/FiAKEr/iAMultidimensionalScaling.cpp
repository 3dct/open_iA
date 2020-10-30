#include "iAMultidimensionalScaling.h"

#include "iAConsole.h"
#include "iAStringHelper.h"

#include <algorithm>

namespace
{

	void initializeRandom(iAMatrixType& result, int rows, int cols)
	{
		result.resize(rows);
		double rand2 = 1.0 / rows;
		for (int r = 0; r < rows; r++)
		{
			result[r] = std::vector<double>(cols, rand2);
			rand2 += (1.0 / rows);
		}
	}

	// assumes quadratic matrix!
	double matrixMean(iAMatrixType const& input)
	{
		return std::accumulate(input.cbegin(), input.cend(), 0,
			[](double lhs, std::vector<double> const& rhs) {
				return std::accumulate(rhs.cbegin(), rhs.cend(), lhs);
			}) / (input.size() * input.size());
	}

	void addNumberSelf(iAMatrixType& input, double value)
	{
		for (auto d : input)
		{
			for (auto& v : d)
			{
				v += value;
			}
		}
	}

	void multiplyNumberSelf(iAMatrixType& input, double value)
	{
		for (auto d : input)
		{
			for (auto& v : d)
			{
				v *= value;
			}
		}
	}


	// DEBUG methods:
	QString matrixToString(iAMatrixType const& input)
	{
		QString output = QString("%1x%2:\n").arg(input.size()).arg(input[0].size());
		for (int r = 0; r < input.size(); ++r)
		{
			output += joinAsString(input[r], ",",
				[](double val) -> QString { return QString::number(val, 'g', 3); })
				+ "\n";
		}
		return output;
	}

	iAMatrixType vectorDiff(iAMatrixType const& a, iAMatrixType const& b)
	{
		iAMatrixType result(a.size(), std::vector<double>(a[0].size(), 0.0));
		for (int i = 0; i < a.size(); ++i)
		{
			for (int j = 0; j < a[0].size(); ++j)
			{
				result[i][j] = std::abs(a[i][j] - b[i][j]);
			}
		}
		return result;
	}

	void computeDistance(iAMatrixType const& dataMatrix, iAMatrixType& distanceMatrix)
	{
		for (int r = 0; r < dataMatrix.size() - 1; r++)
		{
			for (int c = r + 1; c < dataMatrix.size(); c++)
			{
				double temp = 0;
				for (int c2 = 0; c2 < dataMatrix[0].size(); c2++)
				{
					temp += pow(dataMatrix[r][c2] - dataMatrix[c][c2], 2);
				}
				distanceMatrix[r][c] = std::sqrt(temp);
			}
		}
		// fill other half triangle:
		for (int r = 1; r < distanceMatrix.size(); r++)
		{
			for (int c = 0; c < r; c++)
			{
				distanceMatrix[r][c] = distanceMatrix[c][r];
			}
		}
		//DEBUG_LOG(QString("Euclidean Distance:\n%1").arg(matrixToString(distanceMatrix)));
	}
}

/*
QString distanceMetricToString(int i)
{
	switch (i)
	{
	case 0: return QString("Euclidean Distance");
	case 1: return QString("Minkowski Distance");
	default: return QString("Invalid DistanceMetric");
	}
}

iADistanceMetricID stringToDistanceMetric(QString const & string)
{
	if (string == QString("Euclidean Distance"))
	{
		return iADistanceMetricID::EuclideanDistance;
	}
	else if (string == QString("Minkowski Distance"))
	{
		return iADistanceMetricID::MinkowskiDistance;
	}
	else
	{
		return iADistanceMetricID::Unknown;
	}
}

iAMatrixDistance* initializeDistanceMetric(iADistanceMetricID distanceMetric)
{
	if (distanceMetric == iADistanceMetricID::EuclideanDistance)
	{
		return new iAEuclideanDistance();
	}
	else if (distanceMetric == iADistanceMetricID::MinkowskiDistance)
	{
		return new iAMinkowskiDistance();
	}
	return nullptr;
}
*/

std::vector<std::vector<double>> calculateMDS(std::vector<std::vector<double>> const& distanceMatrix,
	int outputDimensions, int iterations/*, iADistanceMetricID distanceMetric*/)
{
	DEBUG_LOG(QString("DistanceMatrix:\n%1").arg(matrixToString(distanceMatrix)));

	//X - configuration of points in Euclidean space
	//initialize X with one vector filled with random values between [0,1]
	
	auto numElems = distanceMatrix.size();
	assert(numElems > 2 && numElems == distanceMatrix[0].size()); // at least 3 elements and quadratic matrix
	iAMatrixType X;
	initializeRandom(X, numElems, outputDimensions);
		
	DEBUG_LOG(QString("init X:\n%1").arg(matrixToString(X)));

	// mean value of distance matrix
	double meanD = matrixMean(distanceMatrix);

	// move to the center
	addNumberSelf(X, -0.5); // why 0.5?
	//addNumberSelf(X, -meanD); // maybe this should be the mean (mean should be 0.5 if numbers in X were chosen from uniform distribution 0..1)

	// before this step, mean distance is 1/3*sqrt(d)
	multiplyNumberSelf(X, 0.1 * meanD / (1.0 / 3.0 * sqrt((double)outputDimensions)));
	DEBUG_LOG(QString("normalized X:\n%1").arg(matrixToString(X)));

	iAMatrixType Z(X);
	DEBUG_LOG(QString("init Z:\n%1").arg(matrixToString(Z)));
	iAMatrixType D(numElems, std::vector<double>(numElems, 0.0));
	iAMatrixType B(numElems, std::vector<double>(numElems, 0.0));

	//calculate euclidean distance
	computeDistance(X, D);
	DEBUG_LOG(QString("D:\n%1").arg(matrixToString(D)));

	const double Epsilon = 0.000001;
	//MDS iteration
	for (int it = 0; it < iterations; it++)
	{
		DEBUG_LOG(QString("\nITERATION %1:").arg(it));
		DEBUG_LOG(QString("B old:\n%1").arg(matrixToString(B)));

		// B = calc_B(D_,D);
		for (int r = 0; r < numElems; r++)
		{
			for (int c = 0; c < numElems; c++)
			{
				if ( r == c || std::fabs(D[r][c]) < Epsilon)
				{
					B[r][c] = 0.0;
				}
				else
				{
					B[r][c] = -distanceMatrix[r][c] / D[r][c];
				}
			}
		}
		DEBUG_LOG(QString("B middle:\n%1").arg(matrixToString(B)));

		for (int c = 0; c < numElems; c++)
		{
			double temp = 0;
			for (int r = 0; r < numElems; r++)
			{
				temp += B[r][c];

			}

			B[c][c] = -temp;
		}
		DEBUG_LOG(QString("B new:\n%1").arg(matrixToString(B)));

		DEBUG_LOG(QString("Z:\n%1").arg(matrixToString(Z)));

		// X = B*Z/size(D,1);
		for (int r = 0; r < X.size(); r++)	
		{
			for (int xCols = 0; xCols < X[0].size(); xCols++)
			{
				double temp = 0;
				for (int bCols = 0; bCols < B[0].size(); bCols++)
				{
					temp += (B[r][bCols] * Z[bCols][xCols]);
				}

				X[r][xCols] = temp / numElems;
			}
		}
		DEBUG_LOG(QString("X:\n%1").arg(matrixToString(X)));

		//D_ = calc_D (X);
		computeDistance(X, D);
		DEBUG_LOG(QString("D:\n%1").arg(matrixToString(D)));

		DEBUG_LOG(QString("diff Z-X:\n%1").arg(matrixToString(vectorDiff(Z, X))));
		Z = X;
	}
	return X;
}
