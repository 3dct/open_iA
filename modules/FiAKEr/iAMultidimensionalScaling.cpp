#include "iAMultidimensionalScaling.h"

#include "iAStringHelper.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace
{

	void initializeRandom(iAMatrixType& result, size_t rows, size_t cols)
	{
		std::mt19937 rng;
		rng.seed(std::random_device{}());
		std::uniform_real_distribution<double> dist;
		result.resize(rows);
		for (size_t r = 0; r < rows; ++r)
		{
			result[r].resize(cols);
			for (size_t c = 0; c < cols; ++c)
			{
				result[r][c] = dist(rng);
			}
		}
	}

	double matrixSum(iAMatrixType const& input)
	{
		return std::accumulate(input.cbegin(), input.cend(), 0.0,
			[](double val, std::vector<double> const& vec)
			{
				return std::accumulate(vec.cbegin(), vec.cend(), val);
			});
	}

	//! mean of a 2D matrix (inequal column/row sizes are accounted for)
	double matrixMean(iAMatrixType const& input)
	{
		size_t num = std::accumulate(input.cbegin(), input.cend(), 0.0,
			[](double val, std::vector<double> const& vec)
			{
				return val + vec.size();
			});
		return matrixSum(input) / num;
	}

	void matrixAddScalar(iAMatrixType& input, double value)
	{
		for (auto& d : input)
		{
			for (auto& v : d)
			{
				v += value;
			}
		}
	}

	void matrixMultiplyScalar(iAMatrixType& input, double value)
	{
		for (auto& d : input)
		{
			for (auto& v : d)
			{
				v *= value;
			}
		}
	}

	void computeDistance(iAMatrixType const& dataMatrix, iAMatrixType& distanceMatrix)
	{	// use/merge with computeDistanceMatrix?
		for (size_t r = 0; r < dataMatrix.size() - 1; r++)
		{
			for (size_t c = r + 1; c < dataMatrix.size(); c++)
			{
				double temp = 0;
				for (size_t c2 = 0; c2 < dataMatrix[0].size(); c2++)
				{
					temp += pow(dataMatrix[r][c2] - dataMatrix[c][c2], 2);
				}
				distanceMatrix[r][c] = std::sqrt(temp);
			}
		}
		// fill other half triangle:
		for (size_t r = 1; r < distanceMatrix.size(); r++)
		{
			for (size_t c = 0; c < r; c++)
			{
				distanceMatrix[r][c] = distanceMatrix[c][r];
			}
		}
		//LOG(lvlDebug, QString("Euclidean Distance:\n%1").arg(matrixToString(distanceMatrix)));
	}


	// DEBUG methods:

	iAMatrixType vectorDiff(iAMatrixType const& a, iAMatrixType const& b)
	{
		iAMatrixType result(a.size(), std::vector<double>(a[0].size(), 0.0));
		for (size_t i = 0; i < a.size(); ++i)
		{
			for (size_t j = 0; j < a[0].size(); ++j)
			{
				result[i][j] = std::abs(a[i][j] - b[i][j]);
			}
		}
		return result;
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

QString matrixToString(iAMatrixType const& input)
{
	QString output = QString("%1x%2:\n").arg(input.size()).arg(input[0].size());
	for (size_t r = 0; r < input.size(); ++r)
	{
		output += joinAsString(input[r], ",",
			[](double val) -> QString { return QString::number(val, 'g', 7); })
			+ "\n";
	}
	return output;
}

std::vector<std::vector<double>> computeMDS(std::vector<std::vector<double>> const& distanceMatrix,
	int outputDimensions, int iterations, double maxError/*, iADistanceMetricID distanceMetric*/)
{
	//LOG(lvlDebug, QString("DistanceMatrix: %1").arg(matrixToString(distanceMatrix)));

	//X - configuration of points in Euclidean space
	//initialize X with one vector filled with random values between [0,1]
	
	auto numElems = distanceMatrix.size();
	assert(numElems > 2 && numElems == distanceMatrix[0].size()); // at least 3 elements and quadratic matrix
	iAMatrixType X;
	initializeRandom(X, numElems, outputDimensions);
	//LOG(lvlDebug, QString("init X: %1").arg(matrixToString(X)));
		
	//LOG(lvlDebug, QString("init X:\n%1").arg(matrixToString(X)));

	// mean value of distance matrix
	double meanD = matrixMean(distanceMatrix);
	//LOG(lvlDebug, QString("mean=%1").arg(meanD));

	// move to the center
	matrixAddScalar(X, -0.5);
	//LOG(lvlDebug, QString("subt X: %1").arg(matrixToString(X)));

	// before this step, mean distance is 1/3*sqrt(d)
	matrixMultiplyScalar(X, 0.1 * meanD / (1.0 / 3.0 * sqrt((double)outputDimensions)));
	//LOG(lvlDebug, QString("normalized X: %1").arg(matrixToString(X)));

	iAMatrixType Z(X);
	iAMatrixType D(numElems, std::vector<double>(numElems, 0.0));
	iAMatrixType B(numElems, std::vector<double>(numElems, 0.0));

	computeDistance(X, D);
	//LOG(lvlDebug, QString("D: %1").arg(matrixToString(D)));

	const double Epsilon = 0.000001;
	//MDS iteration
	double diffSum = 1;
	for (int it = 0; it < iterations && diffSum > maxError; it++)
	{
		//LOG(lvlDebug, QString("ITERATION %1:").arg(it));
		//LOG(lvlDebug, QString("B old: %1").arg(matrixToString(B)));

		// B = calc_B(D_,D);
		for (size_t r = 0; r < numElems; r++)
		{
			for (size_t c = 0; c < numElems; c++)
			{
				if (r == c || std::fabs(D[r][c]) < Epsilon)
				{
					B[r][c] = 0.0;
				}
				else
				{
					B[r][c] = -distanceMatrix[r][c] / D[r][c];
				}
			}
		}

		for (size_t c = 0; c < numElems; c++)
		{
			double temp = 0;
			for (size_t r = 0; r < numElems; r++)
			{
				temp += B[r][c];

			}

			B[c][c] = -temp;
		}
		//LOG(lvlDebug, QString("B new: %1").arg(matrixToString(B)));

		//LOG(lvlDebug, QString("Z: %1").arg(matrixToString(Z)));

		// X = B*Z/size(D,1);
		for (size_t r = 0; r < X.size(); r++)	
		{
			for (size_t xCols = 0; xCols < X[0].size(); xCols++)
			{
				double temp = 0;
				for (size_t bCols = 0; bCols < B[0].size(); bCols++)
				{
					temp += (B[r][bCols] * Z[bCols][xCols]);
				}

				X[r][xCols] = temp / numElems;
			}
		}
		//LOG(lvlDebug, QString("X: %1").arg(matrixToString(X)));

		//D_ = calc_D (X);
		computeDistance(X, D);
		//LOG(lvlDebug, QString("D: %1").arg(matrixToString(D)));

		auto vecDiff = vectorDiff(Z, X);
		diffSum = matrixSum(vecDiff);
		//LOG(lvlDebug, QString("diff Z-X: %1").arg(matrixToString(vecDiff)));
		//LOG(lvlDebug, QString("sum (diff Z-X): %1").arg(diffSum));
		Z = X;
	}
	return X;
}
